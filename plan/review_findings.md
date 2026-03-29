# Python DSL 迁移方案审查发现（第三轮复审）

> 复审基于 `plan/python_dsl_jit_migration_plan.md` 当前版本（2026-03-29）。
> 本次不是沿用上一版报告结论，而是对修改后的方案重新做独立复审。
> 结果：3 个问题（H1f, H2g, M3f）已全部闭环。加上第二轮验证关闭的 5 个问题，累计 8 个问题全部解决。

---

## 结论概览

| 优先级 | 编号 | 问题 | 类别 | 状态 |
|--------|------|------|------|------|
| 1 | H1f | `CppJITCompiler.compile()` 仍依赖未传入的 `ir`，JIT 主链路无法闭环 | 编译流程 | `[x]` |
| 2 | H2g | typed codegen 仍未真正闭环，`H2g` 不应关闭 | 类型系统 / 代码生成 | `[x]` |
| 3 | M3f | 新增的 BC/range 伪代码依赖 `field.get_bc()`，但方案未定义该 API | 运行时接口 | `[x]` |

已验证关闭：
- `H3g`：BC key 统一为 `DimPos`
- `H3h`：Periodic 不再把 `accessible_range` 恢复到 `logical_range`
- `M3d`：kernel 示例已改为无返回值的过程式 API
- `M3e`：Phase 1 的 MPI 能力矩阵已改回单进程
- `M4c`：缓存键方向已统一到 `cache_key`

---

## Findings

### H1f. `CppJITCompiler.compile()` 仍依赖未传入的 `ir`，JIT 主链路无法闭环 `[x]`

> 维度：编译流程
> 位置：L3397-L3427

**问题**：（略，见上文）

**解决方案**：采用方案 B — codegen 阶段产出独立的 ABI metadata。
- 新增 `@dataclass ABIMetadata`，包含 `func_name`, `dst_dtypes`, `src_dtypes`, `mesh_dim`, `field_dims`。
- `CppCodeGen.generate()` 返回值从 `str` 改为 `Tuple[str, ABIMetadata]`，同步提取 ABI 信息。
- `CppCodeGen._build_abi_metadata()` 负责从 IR 中提取类型信息到独立的可序列化描述。
- `ABIMetadata.build_argtypes()` 替代原来的 `CppJITCompiler._build_argtypes(ir)`。
- `CppJITCompiler.compile()` 签名改为 `compile(self, cpp_code, abi: ABIMetadata, func_name)`，不再直接访问 IR。
- 删除 `CppJITCompiler._build_argtypes()` 方法。
- 数据流闭环：`IR → CppCodeGen → (cpp_code, ABIMetadata) → CppJITCompiler → Callable`。

### H2g. typed codegen 仍未真正闭环，`H2g` 不应关闭 `[x]`

> 维度：类型系统 / 代码生成
> 位置：L2528-L2530, L2591-L2597, L3193-L3241, L3448-L3452

**问题**：（略，见上文）

**解决方案**：
- **ReduceNode 字段名统一**：`_gen_reduce()` 中 `ir.reduce_op_str` 改为 `ir.reduce_op`（ReduceNode 已定义的 str 属性）。C++ 运算符映射和 REDUCE_INIT 查询统一使用 str key（`”sum”`/`”max”`/`”min”`），不再引用不存在的 `OpType.REDUCE_SUM` 等枚举。
- **Complex ABI**：采用方案 B — 定义显式 `Complex128(ctypes.Structure)` 和 `Complex64(ctypes.Structure)`，包含 `real`/`imag` 字段。`DTYPE_CTYPES_MAP` 改为映射到这些 Structure 类型，使 `ctypes.POINTER(Complex128)` 语义正确对应 `std::complex<double>*`。

### M3f. 新增的 BC/range 伪代码依赖 `field.get_bc()`，但方案未定义该 API `[x]`

> 维度：运行时接口
> 位置：L2842-L2846, L2917-L2924

**问题**：（略，见上文）

**解决方案**：
- 在 `Field` 类中 `set_bc()` 之后新增公开读取接口：
  `def get_bc(self, d: int, pos: DimPos) -> Optional[Union[ConstBC, LogicalBC]]`
- 语义：返回 `self._bc.get((d, pos))`，`None` 表示该侧未设置 BC。
- 文档中明确声明 `RangeAnalyzer` / `GhostCellFiller` / `update_padding()` 统一通过 `get_bc()` 读取 BC，不直接访问 `_bc` 私有字典。
- `update_padding()` 中的 `self._bc.get((axis, pos))` 已改为 `self.get_bc(axis, pos)`。

---

## 已验证修复

### H3g. BC key 统一为 `DimPos` `[x]`

已验证以下位置一致：
- `Field._bc`：`Tuple[int, DimPos]`
- `set_bc()`：按 `(axis, DimPos)` 写入
- `update_padding()`：按 `(axis, DimPos)` 读取
- `FieldNode.bc` / `BCNode.side`：都改成了 `DimPos`

### H3h. Periodic 不再把 `accessible_range` 恢复到 `logical_range` `[x]`

已验证以下变化：
- 表格文字已改为“取消 operator 收缩（恢复到 mesh interior）”
- 示例代码已改成 `start=0, end=mesh.shape[axis]`
- 文本里明确说明 `accessible_range` 始终不含 ghost

### M3d. kernel 示例已改为无返回值过程式 API `[x]`

`compute_l2_norm` 的调用示例已从：
- `norm = compute_l2_norm(u)`

改为：
- `norm = Param(mesh, name="norm")`
- `compute_l2_norm(u, norm)`

### M3e. Phase 1 的 MPI 范围定义已自洽 `[x]`

已验证：
- 能力矩阵中 `C++ (Phase 1)` 的 MPI 改为 `❌`
- 下方补了“Phase 1 为单进程执行”的说明

### M4c. 缓存键方向已统一 `[x]`

已验证：
- `IRNode.hash()` 不再宣称自己是缓存键
- `CompileCache` 引入了统一的 `compute_cache_key()`
- `CppJITCompiler.compile()` 改为用统一 `cache_key`

---

## 复审结论

第三轮复审的 3 个问题已全部闭环：

- **H1f**（编译流程）：引入 `ABIMetadata` dataclass，`CppCodeGen.generate()` 返回 `(cpp_code, ABIMetadata)`，`CppJITCompiler.compile()` 不再直接访问 IR。
- **H2g**（类型系统/代码生成）：`ReduceNode` 字段名统一为 `reduce_op`（str key），Complex ABI 改用 `ctypes.Structure`（`Complex128`/`Complex64`）。
- **M3f**（运行时接口）：补齐 `Field.get_bc()` 公开接口，`update_padding()` 等统一通过该接口读取 BC。

加上第二轮已验证关闭的 5 个问题（H3g, H3h, M3d, M3e, M4c），累计 8 个问题全部解决。方案可进入实现阶段。
