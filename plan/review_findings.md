# Python DSL 迁移方案审查发现（第二轮）

> 审查基于 `plan/python_dsl_jit_migration_plan.md` 的四维独立审查。
> 状态标记：`[ ]` 待解决 | `[~]` 讨论中 | `[x]` 已解决
> 第一轮审查（28 issues）已全部修复并合并，本轮为第二轮独立审查。

---

## 问题总览

> **全部 12 个问题已解决。** CRITICAL 级（第一轮）+ HIGH 级 + MEDIUM 级全部关闭。

| 优先级 | 编号 | 问题 | 类别 | 状态 |
|--------|------|------|------|------|
| ~~1~~ | ~~H2d~~ | ~~优化 Pass 无框架~~ | ~~前后端解耦~~ | **已解决** |
| ~~2~~ | ~~H2e~~ | ~~新算子需修改 OpType 枚举~~ | ~~前后端解耦~~ | **已解决** |
| ~~3~~ | ~~H2f~~ | ~~新数据类型扩展路径不清晰~~ | ~~前后端解耦~~ | **已解决** |
| ~~4~~ | ~~H3e~~ | ~~`op.reduce()` 完整设计~~ | ~~运行时接口~~ | **已解决** |
| ~~5~~ | ~~H3f~~ | ~~BC 值表示不完整~~ | ~~运行时接口~~ | **已解决** |
| ~~6~~ | ~~H2b~~ | ~~OperatorChecker 约束检查阶段划分~~ | ~~前后端解耦~~ | **已解决** |
| ~~7~~ | ~~M3b~~ | ~~CSE 未设计~~ | ~~优化~~ | **已解决** |
| ~~8~~ | ~~M1a~~ | ~~D1WENO53Upwind stencil 点数差异~~ | ~~C++ 语义~~ | **已解决** |
| ~~9~~ | ~~M3a~~ | ~~类型推导规则硬编码~~ | ~~前后端解耦~~ | **已解决** |
| ~~10~~ | ~~M3c~~ | ~~多后端 IR 兼容性验证无机制~~ | ~~前后端解耦~~ | **已解决** |
| ~~11~~ | ~~M4a~~ | ~~NumPy ↔ C++ stride 处理~~ | ~~编译流程~~ | **已解决** |
| ~~12~~ | ~~M4b~~ | ~~include 头文件管理~~ | ~~编译流程~~ | **已解决** |

---

## 本轮讨论已解决的 HIGH 级问题

### H2d. 优化 Pass 框架 `[x]`

> 维度：前后端解耦 | 位置：Section 6

**决议**：Phase 1 采用最简方案：
- **单一 `PassPipeline`**，显式排序（分析 Pass 排在优化 Pass 前面）
- **显式排序**，不引入声明式依赖管理（Phase 1 Pass <10 个）
- **Per-kernel 粒度**，KernelFusion 推迟到 Phase 2
- Pass 接口：`IRPass.run(kernel_ir: KernelIR) -> KernelIR`

### H2e. OpType 枚举耦合 `[x]`

> 维度：前后端解耦 | 位置：L2199-2258

**决议**：Phase 1 保持单一 `OpType` 枚举，不拆分、不引入注册机制。理由：
- 新增算子改动量极小（~5 行/算子），全部追加式修改
- 真正的多后端扩展性问题应在 `Backend` 抽象层解决，与 OpType 拆分无关
- 枚举拆分引入 union type dispatch 复杂度，Phase 1 收益为零
- Phase 2 如超过 ~80 成员再按语义分组

### H2f. 数据类型扩展 `[x]`

> 维度：前后端解耦 | 位置：L2339-2362

**决议**：引入完整 `DType` 枚举 + `PROMOTE_TABLE` 查表，Phase 1 即支持全部类型：

**DType 枚举**：
| 枚举值 | 简称 | C++ 类型 | 大小 |
|--------|------|---------|------|
| `FLOAT64` | f64 | `double` | 8B |
| `FLOAT32` | f32 | `float` | 4B |
| `COMPLEX128` | c128 | `std::complex<double>` | 16B |
| `COMPLEX64` | c64 | `std::complex<float>` | 8B |
| `INT32` | i32 | `int32_t` | 4B |
| `BOOL` | i1 | `bool` | 1B |

**类型提升规则**（`PROMOTE_TABLE` 查表，遵循 NumPy 语义）：
- 精度不丢失 + real→complex 自动提升
- `f64 + c64 → c128`（f64 精度高于 c64 的 float 实部）
- 损失精度的隐式转型在编译时报 warning

**算子-类型约束**：
- 复数禁止比较运算（`>`, `<`, `>=`, `<=`），`==`/`!=` 允许
- `BOOL` 支持逻辑运算（AND/OR/NOT）和隐式转型（false→0, true→1）
- 隐式类型提升自动发生，损失精度时编译期 warning

**对 IRNode 的改动**：`dtype: str` → `dtype: DType`

### H3e. `op.reduce()` 完整设计 `[x]`

> 维度：运行时接口 | 位置：Section 3.4 / L1627-1676

**决议**：

**一等概念 `Param`（参量）**：
- 定义在网格定义域上的 0 维数据
- 分布式场景下所有进程观察到的值保持一致（allreduce 语义）
- `op.reduce()` 返回 `Param`，不是裸 scalar

**Kernel 无返回值**：
- Kernel 输出全部通过 `assign` 写入 Field 或 Param
- 不允许 `return` 语句

**Reduce 与 Assign 混用（方案 C）**：
- 允许在同一 kernel 中混用 reduce 和 assign
- 允许多个 reduce，各自生成独立循环
- **允许 reduce 之间互相依赖**，通过拓扑排序分阶段执行
- 循环依赖编译期报 `CyclicDependencyError`

**执行模型**：
1. 拓扑排序所有 Param assign，构建依赖 DAG
2. 分阶段执行：无依赖 reduce → 依赖前序结果的 reduce/标量运算 → ...
3. 所有 Param 计算完成后，执行 Field assign 循环

**Phase 1 限制**：
- 仅全局 reduce（整个 accessibleRange），不支持 per-axis reduce
- OpenMP `reduction` 子句并行
- 多 reduce 独立循环，不融合

### H3f. BC 值表示 `[x]`

> 维度：运行时接口 | 位置：L2362-2380

**决议**：BC 值统一为 `Optional[IRNode]`，支持三种形式：

1. **常量 BC**：`BCNode.value = ScalarNode(0.0)`
2. **空间函数 BC**：`BCNode.value = op.sin(mesh.x[0]) * op.exp(-mesh.x[1])`（IR 表达式树引用坐标）
3. **逻辑 BC**：`BCNode.value = None`（Periodic/Symm/ASymm 无值）

**时间变化 BC**：与 C++ 一致——每个时间步调用 `set_bc()` 更新 value，下次 kernel 调用使用新 IR。

**Robin BC 参数**：`robin_a/b/c` 从 `Optional[float]` 升级为 `Optional[IRNode]`，支持空间变化。

**代码生成**：
- ScalarNode value → 内联常量
- 表达式树 value → 递归 `_gen_expr()`，`mesh.x[axis]` 展开为边界坐标

### H2b. 约束检查架构拆分 `[x]`

> 维度：前后端解耦 | 位置：Section 3.2.10 / Section 3.5.4

**决议**：移除 `OperatorChecker` 和 `ConstraintChecker`，拆分为：

**装饰器语法检查**（纯 Python AST，不涉及 IR）：
- `_operator_syntax_check()`：签名格式、参数类型、返回类型、只读约束
- `_kernel_syntax_check()`：参数类型标注、禁止 AST 节点、禁止内置函数、无返回值

**验证 Pass**（全部纳入 PassPipeline）：
- `LocConsistencyPass`：二元运算 loc 一致性（取代 `ExprLocChecker`）
- `TypeInferencePass`：类型推导 + 算子-类型约束
- `ReduceDependencyPass`：Param 依赖 DAG + 循环依赖检测
- `StencilAnalysisPass`：stencil 访问模式 → range_effect
- `RangeAnalysisPass`：range 传播
- `HaloSufficiencyPass`：stencil 所需 halo ≤ Field 实际 halo

**默认 Pipeline**：
```
验证: LocConsistency → TypeInference → ReduceDependency
分析: StencilAnalysis → RangeAnalysis
后端验证: HaloSufficiency
优化: StencilSpecialization → MemoryAccess
```

---

## 本轮讨论已解决的 MEDIUM 级问题

### M3b. CSE `[x]`

**决议**：Phase 1 不在 IR 层做 CSE，依赖 C++ 编译器 `-O2` 优化。理由：
- GCC/Clang `-O2` 自带 CSE 远超手写 IR Pass
- 单 kernel 表达式规模有限，编译器优化足够
- Phase 2 如 Taichi 后端需要，再引入 `CSEPass`
- 标注为 Phase 1 已知限制

### M1a. D1WENO53Upwind stencil 点数 `[x]`

**决议**：方案中 5 点为笔误，已修正为 6 点 flux-based 形式（与 C++ 一致）：
- 6 点 stencil：`i-2, i-1, i, i+1, i+2, i+3`
- `bc_width = 3`（两侧各收缩 3）
- 通量差分形式：先计算 5 个差分 `d1..d5`，再构建 WENO 子模板
- eps 自适应：`1e-6 * max(d1², ..., d5²) + 1e-99`（与 C++ 一致）

### M3a. 类型推导规则硬编码 `[x]`

**决议**：随 H2f 解决。`PROMOTE_TABLE` 查表取代硬编码 `type_order` 列表。

### M3c. 多后端 IR 兼容性验证 `[x]`

**决议**：Phase 1 只有 C++ 后端，推迟到 Phase 2。届时引入 `BackendCapability` dataclass 定义标准能力声明。

### M4a. NumPy ↔ C++ stride `[x]`

**决议**：Phase 1 强制 C order（row-major）。
- `Field.__init__` 中 `np.ascontiguousarray()` 保证
- 生成的 C++ 线性索引 `i * stride + j` 与 C order 一致
- Fortran order 支持推迟到有实际需求时

### M4b. include 头文件管理 `[x]`

**决议**：Phase 1 硬编码固定列表：
- `<cmath>`, `<cstdlib>`, `<algorithm>`, `<omp.h>`, `<complex>`
- Phase 2 如头文件增多，改为按需收集（`Set[str]`）

---

## 已解决的 CRITICAL 级问题

### CR1. 编译管线核心环节缺失 `[x]`

> 维度：编译流程完整性

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| CR1a | `@op.kernel` 编译生命周期 | 执行式 IR 构建 + 延迟编译（装饰期/调用期/缓存） |
| CR1b | IRBuilder 访问者实现 | 执行式构建无需独立 visitor，运算符重载直接构建 IR |
| CR1c | `d()`/`d2()` IR 构建 | 函数调用执行时构建 `UnaryOpNode` |
| CR1d | 嵌套 operator IR 构建 | Python 执行顺序天然保证嵌套正确性 |
| CR1e | `_gen_expr()` 递归代码生成 | 完整实现所有节点类型的 C++ 生成 |
| CR1f | `assign()` 全链路 | `@op.kernel` 为编译单元，6 步全链路 |

### CR2. 关键分析器算法缺失 `[x]`

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| CR2a | StencilAnalyzer | 改为遍历 IR 树（非 Python AST） |
| CR2b | RangeAnalyzer | 正值=收缩，交集组合，递归叠加 |
| CR2c | BC ↔ Range 交互 | 按 BC 类型定义收缩规则 + 编译期校验 |

### CR3. C++ 语义缺失 `[x]`

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| CR3a | `mesh._x` 坐标数组 | 均匀/非均匀网格分别初始化 |

### CR4. IR 设计层面 `[x]`

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| CR4a | 运行时 vs 编译期混淆 | 全部编译期常量，IR hash() 包含这些值 |

---

## 此前已解决的 HIGH 级问题

### H1. 代码生成关键组件 `[x]`

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| H1a | UnaryOpNode C++ 生成 | 递归展开 `expanded_ir` |
| H1b | ConvNode 代码生成 | 嵌套求和循环 |
| H1c | ReduceNode 代码生成 | OpenMP parallel reduction |
| H1d | ctypes 签名构造 | `_build_argtypes()` 按 IR 构造 |
| H1e | 编译缓存 key | C++ 代码哈希 = IR 哈希 |

### H2a. IR 后端特定字段 `[x]`

`kernel: Optional[str]` 已移除。`flags: dict` 保留作为通用优化提示。

### H2c. assign() 别名检测 `[x]`

别名检测移至 Python 侧编译期，生成对应 C++ 分支代码。

### H3a-d. 运行时接口 `[x]`

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| H3a | FieldAccessor | IR 代理对象 + MeshProxy |
| H3b | assignable_range | 所有 Range property 实现 |
| H3c | update_padding() | Python eval + JIT C++ 生成 |
| H3d | _gen_alias_copy() | 临时副本 malloc/memcpy/free |

### H4a. D1Linear 插值 `[x]`

改为 Lagrange 线性插值。

---

## 已解决的 MEDIUM / LOW 级问题（第一轮）

| 编号 | 问题 | 决议摘要 |
|------|------|---------|
| M1b | `_shrink_range_axis` 符号约定 | 改为正值=收缩 |
| M2a | RangeAnalyzer TernaryOpNode/ReduceNode | 三分支取交集/归约继承 |
| M2b | UnaryOpNode.axis 语义 | 补充文档，移除 kernel 字段 |
| M2c | assign() 步骤不一致 | 统一为 5 步 |
| L1 | Section 跳号 | 重编号 |

---

## 已修复的 H-A 一致性问题（审查中即时修复）

| 编号 | 问题 | 修复内容 |
|------|------|---------|
| H-A1 | LocOnMesh 在 IR 中存为字符串 | FieldNode.loc 改为 `List[LocOnMesh]` |
| H-A2 | FieldNode.halo 重复定义 | 统一为 `List[Tuple[int, int]]` |
| H-A3 | BCNode.bc_type 枚举 vs 字符串 | 统一为 `BCType` 枚举 |
| H-A4 | BCType.ASYM vs ASYMM 拼写 | 统一为 `ASYM` |
| H-A5 | GhostCellFiller 缺少 ROBIN BC | 补充 ROBIN BC 分支 |
| H-A6 | assign() 4 步 vs 5 步 | 统一为 5 步 |
| H-A7 | Section 编号重复 | 重编号 |
| H-A8 | Field.__init__ halo 计算 | per-dim `(h_start, h_end)` |
| H-A9 | BC 类型字符串比较 | 改为 `BCType` 枚举比较 |
| H-A10 | OpType 缺少 BC | 改用 `OpType.FIELD_REF` |

---

*审查日期: 2026-02-28*
*审查方法: 四子代理并行审查（C++ 语义一致性 / 内部一致性 / 前后端解耦 / 编译流程完整性）*
*更新日期: 2026-03-29 — 全部问题讨论完毕并解决*
