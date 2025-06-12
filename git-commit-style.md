add     添加文件
feat	新功能
fix	    修改bug
docs	文档修改
style	格式修改
refactor	重构
perf	性能提升
test	测试
build	构建系统
ci	    对CI配置文件修改
chore	修改构建流程、或者增加依赖库、工具
revert	回滚版本
deps    升级依赖
improvement 改进对现有功能进行增强或优化
modify  修改




https://www.conventionalcommits.org/zh-hans/v1.0.0-beta.4/

每个提交都必须使用类型字段前缀，它由一个名词组成，诸如 feat 或 fix ，其后接一个可选的作用域字段，以及一个必要的冒号（英文半角）和空格。
当一个提交为应用或类库实现了新特性时，必须使用 feat 类型。
当一个提交为应用修复了 bug 时，必须使用 fix 类型。
作用域字段可以跟随在类型字段后面。作用域必须是一个描述某部分代码的名词，并用圆括号包围，例如： fix(parser):
描述字段必须紧接在类型/作用域前缀的空格之后。描述指的是对代码变更的简短总结，例如： fix: array parsing issue when multiple spaces were contained in string.
在简短描述之后，可以编写更长的提交正文，为代码变更提供额外的上下文信息。正文必须起始于描述字段结束的一个空行后。
在正文结束的一个空行之后，可以编写一行或多行脚注。脚注必须包含关于提交的元信息，例如：关联的合并请求、Reviewer、破坏性变更，每条元信息一行。
破坏性变更必须标示在正文区域最开始处，或脚注区域中某一行的开始。一个破坏性变更必须包含大写的文本 BREAKING CHANGE，后面紧跟冒号和空格。
在 BREAKING CHANGE: 之后必须提供描述，以描述对 API 的变更。例如： BREAKING CHANGE: environment variables now take precedence over config files.
在提交说明中，可以使用 feat 和 fix 之外的类型。
工具的实现必须不区分大小写地解析构成约定式提交的信息单元，只有 BREAKING CHANGE 必须是大写的。
可以在类型/作用域前缀之后，: 之前，附加 ! 字符，以进一步提醒注意破坏性变更。当有 ! 前缀时，正文或脚注内必须包含 BREAKING CHANGE: description

