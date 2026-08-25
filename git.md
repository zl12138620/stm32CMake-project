# Git 使用教程

> 针对本工程（STM32_CMake_Test）的 Git 操作入门指南。远程仓库：`https://github.com/zl12138620/stm32CMake-project.git`，Windows + PowerShell 环境。

---

## 一、先理解 Git 的基本概念

Git 有 4 个"区域"，理解它们就理解了一大半：

```
工作区(你看到的文件)
   │  git add
   ▼
暂存区(staging area，准备提交的内容)
   │  git commit
   ▼
本地仓库(.git 目录，你电脑上的提交历史)
   │  git push
   ▼
远程仓库(GitHub 上的代码)
```

- **工作区**：你正在编辑的文件。
- **暂存区**：`git add` 之后、`git commit` 之前存放的"待提交内容"。
- **本地仓库**：`git commit` 之后，改动保存在本机 `.git` 里。
- **远程仓库**：`git push` 之后，改动上传到 GitHub。

> 记住这个流程：**改代码 → add → commit → push**，是每天最高频的操作。

### 本项目的两个分支

| 分支 | 用途 |
| --- | --- |
| `master` | 完整项目代码（C 源码、CMake、启动文件等） |
| `main` | README 文档 + skill（GitHub 默认显示的分支） |

**提交前先确认你在哪个分支**：终端会显示 `(master)` 或 `(main)`，也可以随时用 `git branch` 查看。

---

## 二、日常提交流程（最常用）

假设你修改了 `User/main.c`，想提交到 GitHub：

### 1. 查看状态

```powershell
git status
```

会告诉你：哪些文件被修改（`modified`）、新增（`new file`）或删除（`deleted`）。

### 2. 查看具体改了什么

```powershell
git diff              # 查看所有未暂存文件的改动
git diff User/main.c  # 只查看某个文件的改动
```

### 3. 添加到暂存区

```powershell
git add User/main.c      # 添加单个文件
git add .                # 添加当前目录下所有改动（build/ 等已被 .gitignore 忽略）
```

> `git add .` 是最常用的，但要注意 `.gitignore` 已排除了 `build/` 等目录，不用担心把编译产物提交上去。

### 4. 提交（写入本地仓库）

```powershell
git commit -m "修改说明"
```

提交信息要能说明这次改动，例如：
- `"Add LED blink feature"`
- `"Fix GPIO clock configuration"`
- `"Update README with flashing guide"`

### 5. 推送到 GitHub

```powershell
git push
```

第一次推送某分支时用完整写法：

```powershell
git push -u origin master    # -u 记住这次推送的分支，以后直接 git push
git push -u origin main
```

---

## 三、拉取远程更新

别人改了代码（或你在另一台电脑提交过），把远程的更新同步到本地：

### 1. 拉取并合并（推荐）

```powershell
git pull
```

等价于 `git fetch` + `git merge`，把远程最新代码合并到当前分支。

### 2. 只下载不合并

```powershell
git fetch origin        # 下载远程更新到 origin/master，但不改动本地工作区
git log HEAD..origin/master   # 查看远程比本地多了哪些提交
```

---

## 四、分支操作

### 查看分支

```powershell
git branch          # 本地分支（* 标记当前分支）
git branch -a       # 所有分支（含远程 origin/...）
git branch -v       # 分支 + 最近提交
```

### 创建并切换到新分支

```powershell
git checkout -b feature/led    # 创建 feature/led 分支并切过去
```

### 切换分支

```powershell
git checkout master
git checkout main
```

> ⚠️ 切换分支前确认工作区干净（`git status` 无未提交改动），否则改动会跟着你"跑"到别的分支。

### 合并分支

在 `master` 上把 `feature/led` 的改动合并进来：

```powershell
git checkout master          # 先切到要合并到的分支
git merge feature/led        # 把 feature/led 合并进 master
git push                     # 推送
```

### 删除分支

```powershell
git branch -d feature/led          # 删除已合并的本地分支
git push origin --delete feature/led   # 删除远程分支
```

## 五、撤销与回退

### 1. 撤销"还没 add"的修改（工作区）

```powershell
git restore User/main.c    # 丢弃 User/main.c 的未暂存改动，恢复到最后一次提交状态
```

> ⚠️ 这个操作**不可恢复**，改错代码前先确认。

### 2. 撤销"已经 add"的内容（暂存区）

```powershell
git restore --staged User/main.c   # 从暂存区移出，但保留工作区改动
```

### 3. 修改上一次提交的信息（还没 push）

```powershell
git commit --amend -m "新的提交信息"
```

> 会改写最近一次提交。**如果已经 push 了，不要用 amend**（会导致历史不一致）。

### 4. 回退到某个提交（本地）

```powershell
git log --oneline          # 先看提交记录，找到要回退的哈希
git reset --hard HEAD~1    # 回退到上一个提交（丢弃最近一次提交的改动）
git reset --hard 5a3c9e2   # 回退到指定提交哈希
```

> ⚠️ `--hard` 会**删除**工作区改动，谨慎使用。已 push 到远程的提交回退要用 `git revert`。

### 5. 反做某个提交（安全回退，保留历史）

```powershell
git revert <commit哈希>    # 生成一个"反向提交"来抵消目标提交，历史保留
```

---

## 六、常见问题与解决

### 1. 推送被拒绝：`[rejected] non-fast-forward`

远程有新提交，本地落后了。解决：

```powershell
git pull                    # 先拉取合并远程更新
# 如果有冲突，解决冲突后：
git add .
git commit -m "merge remote changes"
git push
```

### 2. 合并冲突（`CONFLICT`）

`git pull` 或 `git merge` 时，同一文件被双方修改会冲突。解决步骤：

1. 用 `git status` 找到冲突文件（显示 `both modified`）
2. 打开文件，会看到冲突标记：
   ```
   <<<<<<< HEAD
   你本地的内容
   =======
   远程的内容
   >>>>>>> origin/master
   ```
3. 手动保留需要的部分，删除 `<<<<<<<`、`=======`、`>>>>>>>` 标记
4. 保存后：
   ```powershell
   git add <冲突文件>
   git commit -m "resolve conflict"
   ```

### 3. 想撤销"已推送的远程提交"

```powershell
git revert <commit哈希>     # 生成反向提交
git push                    # 推送到远程
```

### 4. 提交到了错误的分支

还没推送时，可以把提交挪到另一个分支：

```powershell
git log --oneline -1                      # 记住当前提交哈希
git checkout 正确分支
git cherry-pick <那个提交哈希>            # 把这个提交复制到当前分支
```

### 5. 想忽略某些文件（.gitignore）

编辑项目根目录的 `.gitignore`，加一行即可，例如：

```
*.log
temp/
```

---

## 七、命令速查表

| 目的 | 命令 |
| --- | --- |
| 查看状态 | `git status` |
| 查看改动 | `git diff` |
| 添加文件 | `git add .` |
| 提交 | `git commit -m "说明"` |
| 推送 | `git push` |
| 拉取 | `git pull` |
| 查看历史 | `git log --oneline` |
| 查看分支 | `git branch -a` |
| 创建+切换分支 | `git checkout -b 分支名` |
| 切换分支 | `git checkout 分支名` |
| 合并分支 | `git merge 分支名` |
| 撤销未暂存改动 | `git restore 文件` |
| 撤销已暂存 | `git restore --staged 文件` |
| 回退本地提交 | `git reset --hard 哈希` |
| 反做远程提交 | `git revert 哈希` |

---

## 八、本工程常用操作示例

### 修改代码后推送（master 分支）

```powershell
git status                      # 确认改动
git add .                       # 添加所有改动
git commit -m "Add LED blink"   # 提交
git push                        # 推送
```

### 从 GitHub 克隆（新电脑）

```powershell
git clone https://github.com/zl12138620/stm32CMake-project.git
cd stm32CMake-project
```

### 更新到最新代码

```powershell
git pull
```

### 两个分支都看一下

```powershell
git checkout master   # 代码
git checkout main     # README 文档和 skill
```

> 💡 **黄金习惯**：每次 `git push` 前先 `git status` 确认要提交的内容，`git pull` 前先提交或暂存本地改动，避免冲突。

