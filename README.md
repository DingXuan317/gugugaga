# 作业提交说明（必读）

本仓库是 **征途视觉组培训作业** 的统一提交仓库。

所有作业通过 **Fork + Pull Request（PR）** 的方式提交，**不要**直接往本仓库推代码。

---

## 一、提交流程（5 步）

### 第 1 步：Fork 本仓库

打开 <https://github.com/ZT-Visual-Group-Training/gugugaga> ，点右上角 **Fork** → **Create fork**。

完成后你会有一份自己的副本：`https://github.com/你的用户名/gugugaga`

### 第 2 步：把你自己的副本 clone 到本地

> ⚠️ 注意地址是**你自己的用户名**，不是组织的。

```bash
git clone https://github.com/你的用户名/gugugaga.git
cd gugugaga
```

### 第 3 步：创建你的作业文件夹

在仓库根目录建一个文件夹，命名规则：**年级 + 专业 + 姓名**，例如：

```
26电科张三/
```

以后每次作业都放在这个文件夹里，按作业名再分子文件夹：

```
26电科张三/
  ├── 培训2/
  │   ├── student_demo/
  │   └── opencv_demo/
  └── 培训3/
      └── ...
```

### 第 4 步：提交并推送

```bash
git add .
git commit -m "培训2 - 26电科张三"
git push
```

第一次 push 会让你登录 GitHub，按提示走就行。

### 第 5 步：提 PR

回到网页 `https://github.com/你的用户名/gugugaga`，会看到 **Contribute** / **Compare & pull request** 的黄色按钮，点它：

- 标题写：`培训2 - 26电科张三`
- 说明里写清楚：做了什么、怎么编译、运行结果（可以贴截图）
- 点 **Create pull request**

到这里就提交完成了。

之后负责人会在 PR 里留言，你按留言改完再 `git push` 一次，**PR 会自动更新，不需要重新提 PR**。

---

## 二、本次作业要求（培训2）

1. **student_demo** —— 实现 `Student` 类（存储学生信息、打印信息、修改成绩），代码分文件组织（头文件 / 源文件 / main），用 CMake 构建，并打包成静态库后链接使用。
2. **opencv_demo** —— `main.cpp` 用 OpenCV 读取并显示一张图片，用 CMake 管理工程，用 `find_package` 查找并链接 OpenCV。

两个任务都**必须用命令行编译运行**（不要只点 IDE 的运行按钮），编译成功后把代码上传到这里。

> 具体题目以群里发布的《培训2》文档为准。

---

## 三、要上传什么 / 不要上传什么

### 要上传

- 源代码（`.cpp` / `.hpp` / `.h`）
- `CMakeLists.txt`
- 一份简短的 `README.md`：说明怎么编译、怎么运行、以及运行结果截图（可选但强烈建议）

### 不要上传

- `build/` 之类的编译产物目录
- `.exe` / `.o` / `.a` / `.so` 等二进制文件
- `.vscode/`、`.idea/` 等本地 IDE 配置
- OpenCV、第三方库的源码
- 压缩包（`.zip` / `.rar` / `.7z`）

仓库根目录已经放了 `.gitignore`，上面这些会自动被忽略。

万一还是传上去了也没关系：在 PR 里说一声，或者自己删掉再 push 一次。

---

## 四、常见问题

**Q：push 报 403 / `Permission denied`？**

A：你 clone 的是组织的仓库（`ZT-Visual-Group-Training/gugugaga`）。组织仓库你没有写权限，要 clone **你自己 fork 出来的那份**（`你的用户名/gugugaga`）。

**Q：我的 fork 落后于原仓库怎么办？**

A：在你 fork 的仓库页面点 **Sync fork** → **Update branch**，然后本地执行 `git pull`。

**Q：我改了文件，但 PR 里没变化？**

A：先确认 `git push` 成功了，再确认你改的是这个 PR 对应的分支。

**Q：已经提过 PR 了还能改吗？**

A：能。改完继续 `git commit` + `git push`，PR 会自动更新。

**Q：能不能直接在网页上传文件？**

A：可以（**Add file** → **Upload files**），但不推荐：容易漏文件，路径也容易建错。

**Q：`git commit` 报 `Please tell me who you are`？**

A：一次性配置好身份即可：

```bash
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"
```

---

## 五、有问题

在群里问，或者在 PR / Issue 里留言。
