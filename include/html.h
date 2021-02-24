#include <WString.h>

const char index_html_name[] = "/index.html";
const char *indexPage = R"===(<!DOCTYPE HTML>
<HTML>

<HEAD>
    <META charset="utf-8" content="IE=11.0000" http-equiv="X-UA-Compatible">
    <TITLE>智能电源边缘服务器网络参数配置</TITLE>
    <META http-equiv="Content-Type" content="text/html; charset=GB2312">
    <STYLE type="text/css">
        body,
        html {
            background-color: #C0DEED;
        }

        .main {
            padding-top: 30px;
        }

        .content {
            background: #FFFAFA;
            width: 400px;
            margin: 0 auto;
            padding: 10px 40px;
            border-radius: 5px;
        }

        .content h3 {
            color: #66b3ff;
            text-align: center;
        }

        label {
            display: inline-block;
            width: 200px;
        }

        .tips {
            font-size: 13px;
            color: #f00;
        }

        label span {
            color: red;
        }
    </STYLE>

    <SCRIPT>

        function $(id) {
            return document.getElementById(id);
        }
        function settingsCallback(o) {
            if ($('txtAddr'))
                $('txtAddr').value = o.factoryid;
            if ($('txtVer'))
                $('txtVer').value = o.ver;
            if ($('txtMac'))
                $('txtMac').value = o.mac;
            if ($('txtIp'))
                $('txtIp').value = o.lip;
            if ($('txtSub'))
                $('txtSub').value = o.sub;
            if ($('txtGw'))
                $('txtGw').value = o.gw;
            if ($('txtDns'))
                $('txtDns').value = o.dns;
            if ($('txtServer'))
                $('txtServer').value = o.rip;
            if ($('txtport'))
                $('txtport').value = o.port;
            if ($('txtAddr'))
                $('txtAddr2').value = o.usrid;
            if ($('txtVer'))
                $('txtVer').value = o.ver;
            if ($('txtport'))
                $('txtport').value = o.port;
        }

        function submitFrom() {
            oAddr = document.getElementById('txtAddr').value;
            oAddr2 = document.getElementById('txtAddr2').value;
            otIp = document.getElementById('txtIp').value;
            oSub = document.getElementById('txtSub').value;
            oGw = document.getElementById('txtGw').value;
            oDns = document.getElementById("txtDns").value;
            oServer = document.getElementById('txtServer').value;
            oPort = document.getElementById('txtport').value;
            oTotal = document.getElementById('txtRelayTotal').value;
            var pwdRegex = /^[0-9a-fA-F]+$/;
            if (oAddr == '') {
                alert('智能电源出厂序列号不能为空');
            } else if (oAddr.length != 8) {
                alert('智能电源出厂序列号不能少于8位');
            } else if (!pwdRegex.test(oAddr)) {
                alert('智能电源出厂序列号只能是a~f或A~F之间的英文字母、数字或二者的混合');
            } else if (oAddr2 == '') {
                alert('智能电源用户序列号不能为空');
            } else if (oAddr2.length != 12) {
                alert('智能电源用户序列号不能少于12位');
            } else if (!pwdRegex.test(oAddr2)) {
                alert('智能电源用户序列号只能只能是a~f或A~F之间的英文字母、数字或二者的混合');
            } else if (oServer == '') {
                alert("指向服务器地址不能为空");
            } else if (oServer.length > 64) {
                alert("服务器地址长度不能大于64byte");
            } else if (oPort == '') {
                alert("服务器端口号不能为空");
            } else {
                var xmlhttp = new XMLHttpRequest();
                var requestString = "/?factoryid=" + oAddr + "&usrid=" + oAddr2 + "&lip=" + otIp + "&sub=" + oSub + "&gw=" + oGw + "&dns=" + oDns + "&rip=" + oServer + "&port=" + oPort + "&relaytotal=" + oTotal;
                xmlhttp.open("GET", requestString);
                xmlhttp.onreadystatechange = function () {
                    if (xmlhttp.readyState == 4) {
                        if (xmlhttp.status == 200) {
                            if (xmlhttp.responseText == "Parameter configuration is successful") {
                                alert("您的资料已提交，设备正在重启...");
                            }
                            else {
                                document.getElementById("response").innerHTML = xmlhttp.responseText;
                                alert("您的资料已提交失败");
                            }
                        } else {
                            alert("您的资料已提交失败");
                        }
                    }
                }
                xmlhttp.send(null);
            }
        }

        function submitnochange() {
            var xmlhttp = new XMLHttpRequest();
            var requestString = "/?changed=false";
            xmlhttp.open("GET", requestString);
            xmlhttp.onreadystatechange = function () {
                if (xmlhttp.readyState == 4) {
                    if (xmlhttp.status == 200) {
                        if (xmlhttp.responseText == "Configuration changes abandoned") {
                            alert("已放弃更改配置，设备正在重启...");
                        }
                        else {
                            alert("已放弃更改配置失败");
                        }
                    } else {
                        alert("已放弃更改配置失败");
                    }
                }
            }
            xmlhttp.send(null);
        }

    </SCRIPT>
    <META name="GENERATOR" content="MSHTML 11.00.10570.1001">
</HEAD>

<BODY>
    <DIV class="main">
        <DIV class="content">
            <H3>智能电源边缘服务器网络参数配置</H3>
            <DIV>
                <FORM id="frmSetting" method="post" action="" name="myForm">
                    <P>
                        <LABEL>智能电源出厂序列号:</LABEL>
                        <INPUT name="factoryid" id="txtAddr" type="text" size="16"
                            οnkeyup="value=value.replace(/[^\a-\z\A-\Z0-9]/g,'')"
                            οnpaste="value=value.replace(/[^\a-\z\A-\Z0-9]/g,'')"
                            oncontextmenu="value=value.replace(/[^\a-\z\A-\Z0-9]/g,'')">
                    </P>
                    <P class="tips">a~f或A~F之间的英文字母、数字或二者的混合，固定8位</P>
                    <P>
                        <LABEL>智能电源用户序列号:</LABEL>
                        <INPUT name="usrid" id="txtAddr2" type="text" size="16"
                            οnkeyup="value=value.replace(/[^\a-\z\A-\Z0-9]/g,'')">
                    </P>
                    <P class="tips">a~f或A~F之间的英文字母、数字或二者的混合，固定12位</P>
                    <P>
                        <LABEL for="txtIp">固件版本号:</LABEL>
                        <INPUT name="ver" disabled="disabled" id="txtVer" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtIp">MAC地址:</LABEL>
                        <INPUT name="mac" disabled="disabled" id="txtMac" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtIp">本地IP地址:</LABEL>
                        <INPUT name="lip" id="txtIp" type="text" size="16">
                    </P>
                    <P class="tips">删除本地IP地址内容，不填写，即自动获取IP</P>
                    <P>
                        <LABEL for="txtSub">子网掩码:</LABEL>
                        <INPUT name="sub" id="txtSub" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtGw">默认网关:</LABEL>
                        <INPUT name="gw" id="txtGw" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtGw">DNS服务器:</LABEL>
                        <INPUT name="dns" id="txtDns" type="text" size="16">
                    </P>
                    <P id="server1">
                        <LABEL for="txtServer">指向服务器地址:</LABEL>
                        <INPUT name="rip" id="txtServer" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtIp">服务器端口号:</LABEL>
                        <INPUT name="port" id="txtport" type="text" size="16">
                    </P>
                    <P>
                        <LABEL for="txtIp">分路设备回路数量:</LABEL>
                        <select id="txtRelayTotal" style="width:140px">
                            <option value="0">0</option>
                            <option value="4">4</option>
                            <option value="8">8</option>
                            <option value="12">12</option>
                            <option value="16">16</option>
                        </select>
                    </P>
                    <P>
                        <INPUT id="submit" type="button" value="保存并重启" onclick="submitFrom()">
                        <span class="tips" id="response"></span>
                    </P>
                </FORM>
            </DIV>
            <DIV>
                <FORM id="Quit" method="POST" action="">
                    <P>
                        <INPUT name="nochange" id="nochange" type="text" size="16" style="display: none">
                        <INPUT type="button" value="放弃设置并重启" onclick="submitnochange()">
                    </P>
                </FORM>
            </DIV>
            <div id="errContent"></div>
        </DIV>
    </DIV>
    <SCRIPT type="text/javascript">
        settingsCallback(%s);
    </SCRIPT>
</BODY>

</HTML>)===";

