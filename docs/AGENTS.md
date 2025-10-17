这个项目需要使用MSVC c++，库的话用vcpkg。

这个项目目标根据提供的小红书主播ID host_id=6338567c000000001802db10（我希望你在根目录建立一个config.json，像host_id这种数据我都会填在那里），来建立http请求。这个请求的请求头见"疑似查询直播房间API_live-mall.xiaohongshu.com_message.txt"，不是所有请求头都需要，但是目前你还是完全照抄吧。这个请求的作用是向小红书服务器查询当前主播的直播情况。具体的是否有直播的响应我也分别写在"疑似查询直播房间API_live-mall.xiaohongshu.com_message.txt"中了。如果当前有直播间，那响应中会有类似 room_id=569971301260745429 的字样。当然room_id出现在好几处，你看看你选一个合适的地方、格式读取。
还有一个问题就是请求头中的一些信息是我抓包抓来的。这部分信息有可能会超时，我暂时没想好怎么办。你先在默认的config.json中把我给你提供的请求头和值都抄过去。
我自己用curl试的，只有以下请求头是必须的，你在config.json中用注释标注一下。
curl -i -G 'https://live-mall.xiaohongshu.com/api/sns/red/livemall/app/dynamic/host/info' `
  --data-urlencode 'host_id=6338567c000000001802db10' `
  -H 'Host: live-mall.xiaohongshu.com' `
  -H 'xy-direction: 2' `
  -H 'x-b3-traceid: 5f5fd79b1e180500' `
  -H 'xy-scene: fs=0&point=789' `
  -H 'xy-common-params: fid=17551017151098354944b753319f4dbc9ed5a0f718ae&gid=7cf0f197d8be543d0eb19eed0a20ce0a43ce0e79473594bf776f0c54&device_model=phone&tz=Asia%2FShanghai&channel=jlight&versionName=8.96.0&deviceId=c90c36d9-e241-3a36-9452-e9b6507ab5c2&platform=android&sid=session.1757063893546502696766&identifier_flag=4&project_id=ECFAAF&x_trace_page_current=user_page&lang=zh-Hans&app_id=ECFAAF01&uis=light&teenager=0&active_ctry=CN&cpu_name=qcom&dlang=zh&data_ctry=CN&SUE=1&launch_id=1760686341&origin_channel=jlight&overseas_channel=0&mlanguage=zh_cn&folder_type=none&auto_trans=0&t=1760691066&build=8960817&holder_ctry=CN&did=5e040bd8d68c455a94fb963f2b18e622' `
  -H 'shield: XYAAQAAwAAAAEAAABTAAAAUzUWEe0xG1IbD9/c+qCLOlKGmTtFa+lG434HeuFeRaoRzITjnbAzRZ2t/eRez8N738t+2KY3EgwbGDeOYbvzjXll1eahbRK+h24bLIRCDOZU0DA1' `
  -H 'xy-platform-info: platform=android&build=8960817&deviceId=c90c36d9-e241-3a36-9452-e9b6507ab5c2' `
  --compressed


如果当前存在存在直播间，那我希望你用powershell来运行RTMPDump这个程序来下载直播间的rtmp流媒体。具体rtmpdump的说明在docs/rtmpdump.1.html中。我之前问过你，你给过我命令：rtmpdump -r "rtmp://live.xhscdn.com/live/569971301260745429" -o output.flv --live 这个应该是对的，你再确认一下。如果有什么额外有用的选项，也用上。我希望是下载直播的原始视频，不要其他和视频无关的选项。然后这个rtmpdump -o output.flv不要用这个名字，要用 当前room_id，比如 569971301260745429_rtmp.flv。下载到 downloads/20251017 这种有当前日期的文件夹中。下载前先查询一下，如果已存在同名文件，别覆盖了，加个尾缀，比如569971301260745429_rtmp_1.flv 。
我不太清楚你怎么启动powershell，需要powershell的安装目录吗？我目前只考虑windows平台，你看看怎么运行powershell。