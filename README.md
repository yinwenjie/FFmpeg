# yinwenjie/FFmpeg：同步任务分支

这个分支只存放维护 fork 用的 GitHub Actions，不含 FFmpeg 源码。
FFmpeg 源码请看 [`master`](../../tree/master) 分支。

`master` 是官方 [FFmpeg/FFmpeg](https://github.com/FFmpeg/FFmpeg) 的纯镜像，
由 [`.github/workflows/sync-upstream.yml`](.github/workflows/sync-upstream.yml)
每天自动快进同步一次。不要往 `master` 直接提交，否则同步会因无法快进而报错。

GitHub 的定时任务只在默认分支上运行，所以本分支被设为仓库的默认分支。

手动触发：Actions → Sync master from upstream → Run workflow。
