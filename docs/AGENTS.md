这个项目需要使用MSVC c++，库的话用vcpkg，目标平台只有winddows。

这个项目目标根据提供的小红书主播ID host_id=6338567c000000001802db10（我希望你在根目录建立一个config.json，像host_id这种数据我都会填在那里），来建立http请求。这个请求的请求头见"疑似查询直播房间API_live-mall.xiaohongshu.com_message.txt"。这个请求的作用是向小红书服务器查询当前主播的直播情况。具体的是否有直播的响应我也分别写在"疑似查询直播房间API_live-mall.xiaohongshu.com_message.txt"中了。你可以用这样的逻辑：如果当前有直播间，那响应中会有类似 room_id=569971301260745429 的字样。当然room_id会出现在好几处，我希望你现在用这种逻辑：提取响应中所有的 room_id，打印出来。但是无论如何使用你提取到的第一个room_id。
还有一个问题就是请求头中的一些信息是我抓包抓来的。这部分信息有可能会超时，我暂时没想好怎么办。你先在默认的config.json中把我给你提供的请求头和值都抄过去。

如果当前存在存在直播间，那我希望你用控制台来运行RTMPDump这个程序来下载直播间的rtmp流媒体，命令：rtmpdump -r "rtmp://live.xhscdn.com/live/569971301260745429" -o output.flv --live 然后这个rtmpdump -o output.flv不要用这个名字，要用 当前room_id，比如 569971301260745429_rtmp.flv。下载到相对路径 downloads/20251017 这种有当前日期的文件夹中，日期格式YYYYMMDD。下载前先查询一下，如果已存在同名文件，别覆盖了，加个尾缀，比如569971301260745429_rtmp_1.flv 。
