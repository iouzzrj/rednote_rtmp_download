host_id:

5b687ad9c39aaf000120eb98    karin    
5c0a4fb10000000006012fde    shiyu


直播链接
http://live-source-play-hw.xhscdn.com/live/569969586455224067.flv
http://live.xhscdn.com/live/569969586455224067.flv
http://live-source-play-bak-hw.xhscdn.com/live/569969586455224067.flv
http://live-source-play.xhscdn.com/live/569971033624755876.flv


ttmpdump直接输出的flv，ffplay能播放，但是VLC不识别flv中的HEVC编码，需要之后转码
rtmpdump -r "rtmp://live.xhscdn.com/live/569971301260745429" -o output.flv --live

让ffmpeg转码，VLC可以播放
ffmpeg -hide_banner -fflags +genpts -probesize 100M -analyzeduration 100M -i output.flv -c copy out.mkv


curl -i -G 'https://live-mall.xiaohongshu.com/api/sns/red/livemall/app/dynamic/host/info' `
  --data-urlencode 'host_id=5b687ad9c39aaf000120eb98' `
  -H 'Host: live-mall.xiaohongshu.com' `
  -H 'xy-direction: 2' `
  -H 'x-b3-traceid: a5a591c7d3190200' `
  -H 'x-xray-traceid: ccf6e3e2cd721ac95cddb61e303ab50e' `
  -H 'xy-scene: fs=0&point=789' `
  -H 'x-legacy-did: c90c36d9-e241-3a36-9452-e9b6507ab5c2' `
  -H 'x-legacy-fid: 17551017151098354944b753319f4dbc9ed5a0f718ae' `
  -H 'x-legacy-sid: session.1757063893546502696766' `
  -H 'x-mini-gid: 7cf138a2a6d0543d0eb19cf80a20ce0a43cec3d9473591bb778637a1' `
  -H 'x-mini-s1: ABoCAAABSuZRo+Npn7Oc4EcJ1h/0MXmFHz1vMbUfJTqF2vdkdjmsxyDr8WDyTrnH2KJvhD3+WScYNx7uG+A=' `
  -H 'x-mini-sig: 7ca6e39a1cc6ab48b7fe3b83b4408a2582ef9b02a7d462d2ac4dd145d05346b3' `
  -H 'x-mini-mua: eyJhIjoiRUNGQUFGMDEiLCJjIjo1MzgsImsiOiJlMmE3OWJkYzYwYmNlYWU5MTRiZTAxNGYxOWRmMmI0NDAxMjRkNTYxY2I2ZmY2MzM4NDYyY2QxNzNmMWQ5NzY0IiwicCI6ImEiLCJzIjoiNzNjZmJkN2M3YjcyMTcxMjRlMzNjMjdkNjdlNTFhMjk0NzQzODg4NjYxNjAyOTEwZDI1YWY0NTVhNWRlYzgwMzk3ODYwYjk3OTJhNzIwN2EzOWIxZTg4ODUyYmIyNDQ5ZjVlODg0YjhjMDhmNDMxZjdhMzBkNGY3OWQwYjk2MmIiLCJ0Ijp7ImMiOjE0NTIsImQiOjYsImYiOjAsInMiOjQwOTgsInQiOjE1NDYyMDY5MCwidHQiOlsxXX0sInUiOiIwMDAwMDAwMDgwMzhjMDlkYzIyZDk2Y2U4ZjlhYjM2MDQ3MDgzZTVhIiwidiI6IjIuOS4yNzMxIn0.XAAkJHGiTGfOlnwbKoepi6z-oaikl8LxxWc1rjMRMe1_rW4nm41CRIIMvbbZwq8ClxndC5WnG0OeRvFZgDbcCT-vOZ5Gft3B_6TTOHa_qpk5YHRFktc5gip8ExLYNmo9OAA7ClpIWjVsntaL6Wy0_hNsj-jCFdYJR9kfz0EHGrjfNytDNsdE7yiS6__noWkQEml22X-TP2ZrOZWVKSAEaNzEWPOcZboxdNGdbb51p74i7lzbCHAfBOye0VeI2pH8XKXrNl3x4Qb5VtHq6xyME7hBIdvrL7cgebEwiCyV5gjoM0036dO9QmNWOQ6RrW2S82S_LFvu2RDtKgl57XBv2Uno6RhxKAeVuQhA86egNMj4EhL2bb14WRqvw_ZGjgFZSr-Ro75upY3dZh159undfNY2hefxE-dhl64P6jJxYHKoZ3GZFexF2W-SePqGUgyLMUTZ4b74pgHxqOIFohCYoJBtJkVDwGP2RkMfP5qvGUZi-mROKufyohymVLi1KJy_5CmENRAEbMwedw9lbDKlSGnA6L--JV6xKywH9ziQ6fkTUaBr7kWlV8ZpoS4HWS1QLZ-77zZxHEl_GwqnAc1AQ1GA8gR3zXE83XHC3PKjQIR-Sir5LSgz_fLfyQ1miTmalhJW6Hp9a9s269P_I1Ph2WfxCIojOyexlHtcSO1QK_OqP6D_OePRCTF5ZdAohKmsYod6thaxZt39kd1DBiQJ3wEJVth55BWcLNn-MUbkjN2KOwtFyfrojAatGnizOwzpAqZ-6NjaKMTwbqzuMn1KG0r7GAaq53-oo2vNfOXXJvJDOyWHwpyOqWckyiiY8GxaIU2VzrvjFCHHFOb5aJ7Z5W_V_Zr7p5DnzIwhI3jUp9Y9jOO1I3IbUQ9q8p0MvdrsYfqnDomohlRfogAD3Hv3F2CrE8-25b5SmyCSgs_zzgXtTF_uZdax8-ol-IoksR9OKmvGvY0NFP4o85ZVbHoIH346V25_yF21j-nMD5smvB7I75iRkm2UezVNxUwB_znMP5ABtt_CtBrf_E-NVbCeiBaXrvnLXfTCrcG1YgUclAIY5dliUjMbFwCEvGPMJJToa7YyxQFdQP_TYWgf4N1pZeQeVwC7uQ2hkFjcsDc3Le0fDG-5i7ceuP-7wsls4RBLm5JSHL0Cg3cfQSSr5NJmdhj8JHGp-4ZrijtjgWN76EoAfpkw2fzjAs4TidGxm5Tt1zmTGYqXjmJEaXl0SrN4HZblNc-3lDGaRyYQJOSA8MpqtvmEVOGpzuSgBG_5912Br1t52xIhYuHK8uG2_hLr6nDMlAw0vWghi1vybzXc4_QC55MbNu2ESTifedm1G1NvOkyMfRzJyI-C8q15o1O2YA.' `
  -H 'xy-common-params: fid=17551017151098354944b753319f4dbc9ed5a0f718ae&gid=7cf138a2a6d0543d0eb19cf80a20ce0a43cec3d9473591bb778637a1&device_model=phone&tz=Asia%2FShanghai&channel=jlight&versionName=8.96.0&deviceId=c90c36d9-e241-3a36-9452-e9b6507ab5c2&platform=android&sid=session.1757063893546502696766&identifier_flag=4&project_id=ECFAAF&x_trace_page_current=user_page&lang=zh-Hans&app_id=ECFAAF01&uis=light&teenager=0&active_ctry=CN&cpu_name=qcom&dlang=zh&data_ctry=CN&SUE=1&launch_id=1760626617&origin_channel=jlight&overseas_channel=0&mlanguage=zh_cn&folder_type=none&auto_trans=0&t=1760630914&build=8960817&holder_ctry=CN&did=5e040bd8d68c455a94fb963f2b18e622' `
  -H 'user-agent: Dalvik/2.1.0 (Linux; U; Android 14; XQ-DE72 Build/67.1.F.2.220) Resolution/1080*2520 Version/8.96.0 Build/8960817 Device/(Sony;XQ-DE72) discover/8.96.0 NetType/Unknown' `
  -H 'shield: XYAAQAAwAAAAEAAABTAAAAUzUWEe0xG1IbD9/c+qCLOlKGmTtFa+lG434HeuFeRaoRzITjnbAzRZ2t/eRez8N738t+2KY3EgwbGDeOYbvzjXll1eavkQIMLlGIVknPRCjorBrw' `
  -H 'xy-platform-info: platform=android&build=8960817&deviceId=c90c36d9-e241-3a36-9452-e9b6507ab5c2' `
  -H 'referer: https://app.xhs.cn/' `
  -H 'accept-encoding: gzip, deflate' `
  --compressed
