# OpFlow 核心计算 CUDA 迁移详细方案（设计稿）

## 1. 目标与范围

### 1.1 总体目标
将当前项目中以 CPU 为主的核心计算链路迁移为“CPU/GPU 双后端”架构，并在 CUDA GPU（重点覆盖 Ada 及更新架构）上稳定支持以下能力：
- 表达式计算（包括逐点算子、差分/卷积类 stencil、条件表达式与归约）。
- 方程组装（离散算子、边界条件、稀疏矩阵/线性算子构建）。
- 方程求解（线性与非线性迭代主流程，支持已有 MPI 并行框架下的 GPU 加速）。

### 1.2 非目标（本阶段不做）
- 不改变数学模型与离散格式语义。
- 不在第一阶段强制替换现有全部 CPU Solver，仅先提供 GPU 路径与对齐验证。
- 不在第一阶段引入跨供应商后端（如 HIP/SYCL），但保留抽象层可扩展性。

### 1.3 成功判定（高层）
- 功能正确性：GPU 与 CPU 在可接受误差范围内一致。
- 性能收益：核心算子/求解流程在代表性案例上相对 CPU 达到明确加速门槛（如 3x~20x，按问题规模分级）。
- 可维护性：新增后端遵守统一接口，支持回退与混合执行。

---

## 2. 现有架构抽象与迁移原则

### 2.1 抽象分层
建议将执行体系分为四层：
1. **前端语义层（Expression/Equation DSL）**：表达“算什么”。
2. **中间表示层（IR/Execution Plan）**：表达“以何拓扑和依赖执行”。
3. **后端运行层（CPU/OpenMP/CUDA/MPI）**：表达“在哪儿、如何执行”。
4. **通信与I/O层（MPI/HDF5/VTK等）**：表达“如何交换与输出”。

### 2.2 迁移原则
- **语义不变，执行可替换**：DSL 不感知 CUDA 细节。
- **数据主权清晰**：每个 Field/Matrix/Vector 明确 owner（Host/Device/Unified）。
- **最小化同步**：默认异步流执行，必要时显式 fence。
- **可回退**：任何 GPU 路径应允许切回 CPU 参考实现。
- **先算子后求解器**：先打通表达式与 stencil 核心路径，再扩展至完整求解流程。

---

## 3. 数据结构组织方案

### 3.1 Field/Array 内存布局
- 统一内部采用 **SoA 优先**（多分量物理量可按分量分离）以提升合并访问效率。
- 对规则网格字段：
  - 主数据区（interior）与 halo 区逻辑分离。
  - 提供 `View`（只读/可写、切片、子域、带步长）与 `OwningBuffer`（实际分配）分离设计。
- 对 AMR/分块网格：
  - 使用 block/chunk 粒度组织，避免巨型不规则索引。
  - 元数据（层级、邻接、偏移）紧凑化并可常驻 device。

### 3.2 稀疏结构（方程组装/求解）
- 统一 CSR/ELL/HYB 抽象接口，实际格式可按算子特征自动选择：
  - 规则 stencil（5/7/9/27 点）优先“隐式矩阵（matrix-free）”或 stencil 专用格式。
  - 非结构化/复杂边界保留 CSR。
- 引入 `LinearOperator` 抽象：
  - `apply(x, y)` 与 `build/preconditioner` 解耦。
  - 便于 CPU/GPU 路径共享迭代求解框架。

### 3.3 表达式 IR（建议新增）
- 将模板表达式树在运行前降级为轻量 IR：
  - 节点：Unary/Binary/Ternary、Stencil、Reduction、BoundaryOp。
  - 属性：读写集合、数据依赖、访问 pattern、可融合标签。
- IR 用于：
  - kernel 融合决策。
  - stream 调度与依赖图构建。
  - 自动插入 halo exchange/同步点。

---

## 4. 内存组织与迁移策略

### 4.1 内存域与生命周期
定义三类内存域：
- `HostResident`：CPU 常驻。
- `DeviceResident`：GPU 常驻（默认核心计算对象）。
- `UnifiedManaged`：仅用于原型与调试，不作为高性能默认路径。

为每个对象维护状态机：
- Dirty bit：`host_dirty/device_dirty`。
- 最后写入流与事件句柄。
- 当前分布（单 GPU / 多 GPU 分片）。
- 状态不变式：
  - 常态下仅允许 `{host_dirty, device_dirty}` 为 `{1,0}`、`{0,1}` 或 `{0,0}`。
  - `{1,1}` 仅允许出现在带事件追踪的短暂 in-flight 拷贝窗口；若超出窗口则视为运行时错误。

### 4.2 分配策略
- 使用 `cudaMallocAsync + memory pool`（CUDA 11.2+）降低分配开销与碎片。
- 大对象长期驻留，临时缓冲区走池化 allocator。
- 对通信 buffer 使用 `pinned host memory`，并支持可选 `GPUDirect RDMA`。

### 4.3 数据迁移策略
- **显式迁移优先**：在执行计划阶段确定 H2D/D2H，避免隐式 page fault。
- **同步语义明确化**：
  - `device_dirty -> host` 只能由执行计划显式插入同步操作触发，不允许按需隐式迁移。
  - Host 侧访问 device 最新数据前必须完成计划内 `sync_to_host`，否则在 debug 模式触发断言并记录上下文。
- 异步传输 + 流并发：
  - `compute stream` 与 `comm stream` 分离。
  - 双缓冲/环形缓冲覆盖 halo 往返。
- 提供迁移审计日志（debug 级）：统计 bytes、次数、阻塞点。

### 4.4 多 GPU 数据分布
- 默认沿主分解维度做子域切分（与 MPI decomposition 对齐）。
- 每 GPU 持有 interior + halo。
- halo 更新路径：
  - 单机多卡优先 `cudaMemcpyPeerAsync`/NVLink。
  - 跨节点优先 CUDA-aware MPI + GPUDirect。

---

## 5. 表达式组装、Kernel 粒度与调用策略

### 5.1 编译与运行时协同
- 编译期保留模板推导优势（类型、维度、边界策略）。
- 运行时依 IR 决定：
  - 是否融合。
  - launch 参数（block size、shared memory）。
  - 执行拓扑（逐点核 / stencil 核 / reduction 核）。

### 5.2 kernel 粒度策略
- **细粒度模式**：每个表达式节点一个 kernel，开发快、易调试。
- **融合模式（默认生产）**：
  - 将连续逐点表达式融合，减少全局内存往返。
  - stencil 与逐点后处理可选择局部融合（防止寄存器爆炸）。
- **分块策略**：
  - 规则网格：2D/3D tile。
  - 高阶 stencil：共享内存 tile + halo preload。

### 5.3 调用与调度
- 引入 `ExecutionContext`：
  - 管理 stream、事件、handle（cuBLAS/cuSPARSE/NCCL）。
  - 维护 kernel cache（按 shape + dtype + op signature）。
- 提供图执行选项：
  - 高频重复时间步可 capture 为 CUDA Graph，降低 launch overhead。

### 5.4 边界条件与稀疏组装
- 边界处理采用两种策略：
  1. 内核内分支（简单但可能发散）；
  2. interior/boundary 分离核（推荐，减少 warp divergence）。
- 稀疏组装：
  - 优先 matrix-free apply。
  - 必须显式矩阵时，使用并行 prefix-sum + scatter 构建 CSR。

---

## 6. 面向 Ada 及更新架构的优化策略

### 6.1 通用优化基线
- 合并访问、减少非对齐与随机访存。
- 尽量提升算术强度（融合、重用）。
- 控制寄存器使用，维持合理 occupancy（非盲目追高）。

### 6.2 Ada 重点
- 使用 Nsight Compute 关注：
  - DRAM Throughput、L2 hit rate、warp stall 原因。
- 对 stencil 类核：
  - 评估 shared memory 与 L1/L2 缓存路径收益。
  - 调优 tile 形状匹配 SM 资源。
- 对稀疏 SpMV/迭代求解：
  - 优先调用 cuSPARSE/AMGX（若许可）或自研 warp-specialized kernel。

### 6.3 新架构前向兼容
- CMake 增加多 `-gencode`（如 `sm_89`, `sm_90`）并保留 PTX fallback。
- 核心参数采用 runtime autotune + 持久化缓存（按 GPU UUID + 问题形状）。
- 将架构特化逻辑封装在 backend tuning 层，避免污染上层语义代码。
- 预留 Tensor Core 演进路径：初期以 FP32/FP64 为基线，在可证明收敛与精度可控后，再针对特定算子/求解环节评估 TF32/FP16/BF16 混合精度。

---

## 7. 方程求解链路迁移方案

### 7.1 线性求解器路线图
- Phase A：
  - 先接入 GPU 友好 Krylov（CG/BiCGStab/GMRES）+ Jacobi/ILU0（可选库实现）。
- Phase B：
  - AMG 预条件（可对接 AMGCL GPU 后端或外部库）。
- Phase C：
  - matrix-free + 几何多重网格（针对规则网格场景）。

### 7.2 非线性与时间推进
- Newton/Picard 外层保留在主机侧调度，残差/雅可比 action 在 GPU。
- 时间推进（显式/半隐式）按 step graph 执行，跨步重复图建议用 CUDA Graph。

### 7.3 数值一致性策略
- 明确浮点策略：
  - 支持 FP64 基线；
  - 可选混合精度（FP32 compute + FP64 accumulate）并以收敛准则守护。
- 定义对齐指标：残差范数、迭代步、关键物理量守恒误差。

---

## 8. 多 GPU 与多机 MPI 自动并行策略

### 8.1 并行模型
采用 **MPI + CUDA** 主模型：
- 进程级：MPI rank。
- 设备级：每 rank 绑定 1 GPU（首期），后续支持多 GPU/rank。
- 域分解与现有 MPI 策略兼容，新增 GPU 感知调度。

### 8.2 自动并行策略（Auto Policy）
输入：网格形状、方程类型、GPU 数、网络拓扑、显存容量。
输出：
- 分解维度与块大小。
- halo 厚度与交换频率。
- 计算/通信 overlap 策略。

策略实现建议：
1. 启发式初值（规则网格优先长边切分，最小表面积原则）。
2. 运行时采样（若干步）测量通信占比。
3. 在线调整分块或 halo pipeline 深度。

### 8.3 通信优化
- 优先 CUDA-aware MPI。
- 同节点启用 P2P/NVLink，跨节点启用 GPUDirect RDMA（环境满足时）。
- 采用 interior-first + boundary-late 计算顺序实现 overlap。

### 8.4 负载均衡
- 静态均衡：基于网格单元计数。
- 动态均衡：AMR 场景按 block 迁移（重分区周期可配置）。
- 收敛目标：最大 rank 时间 / 平均 rank 时间 < 阈值（如 1.1）。

---

## 9. 功能回归与性能回归测试方案

### 9.1 功能测试分层
1. **单元级**：
   - 表达式节点（算子正确性、边界行为）。
   - kernel 结果与 CPU reference 比较。
2. **组件级**：
   - Field 更新、stencil、reduction、CSR 组装。
3. **集成级**：
   - 典型 PDE 算例（Poisson、对流扩散、LevelSet）。
4. **并行级**：
   - MPI + GPU halo 一致性，跨节点稳定性。

### 9.2 对比与容差
- L∞、L2 相对误差。
- 迭代收敛曲线形状一致性（允许浮点噪声差异）。
- 对关键 benchmark 固化“黄金结果”快照。

### 9.3 性能回归门禁
- 为关键场景建立 baseline（按 GPU 型号与问题规模）。
- CI nightly 跑性能集，触发阈值报警（如退化 >10%）。
- 记录指标：
  - 端到端耗时。
  - kernel 时间分解。
  - H2D/D2H bytes。
  - MPI 通信时间占比。

---

## 10. 性能评测方法学

### 10.1 基准集合
- Microbench：逐点、stencil、reduction、SpMV。
- Application bench：Poisson、FTCS、LidDriven、AMR 代表案例。
- 规模梯度：小/中/大（覆盖 cache-fit 到内存带宽受限区间）。

### 10.2 指标体系
- 吞吐：cells/s、DOF/s。
- 延迟：单步耗时、迭代耗时。
- 效率：
  - 单 GPU 加速比（vs CPU OMP）。
  - 多 GPU 并行效率。
  - 强扩展/弱扩展曲线。
- 硬件计数：SM 占用、访存带宽、stall breakdown。

### 10.3 评测规范
- 固定随机种子、固定编译参数。
- 预热 + 多次重复（报告均值/方差/95% CI）。
- 将 profile 产物（Nsight Systems/Compute）归档，支持版本对比。

---

## 11. 工程落地计划（里程碑）

### 11.1 估算假设与缓冲
- 团队规模假设：2~3 名全职开发（其中至少 1 名具备 CUDA 迁移经验）。
- 依赖复用假设：优先复用 cuSPARSE/现有求解器框架，不从零实现全套稀疏算子库。
- 并行投入假设：测试与性能分析与功能开发并行推进，而非串行收尾。
- 进度缓冲：M1~M3 默认预留约 25%~35% 缓冲，用于调参与跨平台问题消解。

### M0：准备期（1~2 周）
- 增加 CUDA backend 骨架、ExecutionContext、基础 allocator。
- 建立 CPU/GPU 对比测试模板。

### M1：表达式与基础算子（4~8 周，含缓冲）
- M1a（1~2 周）：表达式 IR 降级与调度骨架打通。
- M1b（1~2 周）：逐点表达式 + reduction kernel GPU 化并建立对齐测试。
- M1c（1~2 周）：常见 stencil kernel GPU 化与 tile 参数初调。
- M1d（1~2 周）：单 GPU 单机主链路集成与回归门禁接入。

### M2：方程组装与线性求解（5~9 周，含缓冲）
- M2a（1~2 周）：matrix-free `apply` 与 `LinearOperator` GPU 接口收敛。
- M2b（1~2 周）：CSR 路径与并行组装链路打通。
- M2c（2~3 周）：Krylov + 基础预条件 GPU 化与数值收敛对齐。
- M2d（1~2 周）：求解器性能基线固化与回归阈值接入。

### M3：MPI 多 GPU（3~5 周）
- halo exchange GPU 直连。
- 自动并行策略 v1（启发式 + 采样反馈）。

### M4：优化与稳态（持续）
- Ada/新架构 autotune。
- 性能回归门禁、文档与运维手册完善。

---

## 12. 风险与缓解

- **风险1：表达式过度融合导致寄存器压力过高**
  - 缓解：引入融合成本模型（寄存器估算、算子复杂度阈值）。
- **风险2：MPI+GPU overlap 实际收益不足**
  - 缓解：改进分块顺序与通信批次，必要时采用拓扑感知分解。
- **风险3：跨架构性能波动大**
  - 缓解：autotune + 结果缓存，关键 kernel 多版本实现。
- **风险4：数值差异引发回归噪声**
  - 缓解：定义分层容差与统计准则，保留 FP64 参考路径。
- **风险5：团队 CUDA 学习曲线导致早期生产率波动**
  - 缓解：安排定向培训与结对开发；提前沉淀 CUDA 编码/调试/Nsight 分析规范；将首批 PoC 作为强制上手关卡。

---

## 13. 交付物清单（设计阶段）

1. CUDA 后端总体设计文档（本稿）。
2. 数据结构与内存状态机设计说明（含状态图）。
3. 表达式 IR 与 kernel 融合规则说明。
4. MPI 多 GPU 自动并行策略说明与评测计划。
5. 功能回归 + 性能回归测试规范。

---

## 14. 建议的首批 PoC 切入点

- PoC-1：规则网格 3D 7-point stencil + reduction（单 GPU）。
- PoC-2：Poisson 方程 matrix-free CG（单 GPU）。
- PoC-3：MPI 两卡 halo overlap（同节点 NVLink 或 PCIe）。

通过以上 PoC 可快速验证：
- 数据布局与 kernel 粒度策略是否正确；
- 迁移与通信是否成为瓶颈；
- 在 Ada 架构上是否达到预期加速。
