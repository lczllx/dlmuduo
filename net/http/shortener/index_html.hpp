#ifndef INDEX_HTML_HPP
#define INDEX_HTML_HPP

static const char kIndexHtml[] = R"raw(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
    <title>短链接服务 · C++ Reactor</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            background: linear-gradient(145deg, #f0f4ff 0%, #e6edf5 100%);
            font-family: 'Inter', system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, sans-serif;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 1.5rem;
        }

        .card {
            max-width: 600px;
            width: 100%;
            background: rgba(255, 255, 255, 0.96);
            backdrop-filter: blur(0px);
            border-radius: 2rem;
            box-shadow: 0 20px 35px -12px rgba(0, 0, 0, 0.2), 0 1px 2px rgba(0, 0, 0, 0.02);
            padding: 2rem 2rem 2.2rem;
            transition: all 0.2s ease;
            border: 1px solid rgba(255, 255, 255, 0.5);
        }

        h1 {
            font-size: 1.8rem;
            font-weight: 700;
            background: linear-gradient(135deg, #1E2A5E, #3B4E9E);
            background-clip: text;
            -webkit-background-clip: text;
            color: transparent;
            letter-spacing: -0.3px;
            display: inline-block;
        }

        .badge {
            background: #eef2ff;
            border-radius: 40px;
            padding: 0.2rem 0.8rem;
            font-size: 0.7rem;
            font-weight: 500;
            color: #2d3a8c;
            margin-left: 0.75rem;
            vertical-align: middle;
            display: inline-block;
        }

        .sub {
            color: #5b6b8f;
            margin-top: 0.5rem;
            margin-bottom: 2rem;
            font-size: 0.9rem;
            border-left: 3px solid #818cf8;
            padding-left: 0.75rem;
        }

        .input-group {
            margin: 1.8rem 0 1rem;
        }

        label {
            font-weight: 500;
            font-size: 0.85rem;
            color: #1e293b;
            display: block;
            margin-bottom: 0.4rem;
        }

        input {
            width: 100%;
            padding: 0.9rem 1.2rem;
            font-size: 1rem;
            border: 1.5px solid #e2e8f0;
            border-radius: 1.5rem;
            background: #ffffff;
            transition: 0.2s;
            font-family: inherit;
            outline: none;
        }

        input:focus {
            border-color: #6366f1;
            box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.2);
        }

        button {
            width: 100%;
            background: #1e293b;
            color: white;
            border: none;
            padding: 0.9rem 1.2rem;
            font-size: 1rem;
            font-weight: 600;
            border-radius: 1.8rem;
            cursor: pointer;
            transition: all 0.2s ease;
            margin-top: 0.5rem;
            font-family: inherit;
            background: linear-gradient(95deg, #1E2A5E, #2d3a7c);
            box-shadow: 0 8px 18px -8px rgba(0, 0, 0, 0.2);
        }

        button:hover {
            transform: translateY(-1px);
            background: linear-gradient(95deg, #141f4d, #25337a);
            box-shadow: 0 14px 24px -10px rgba(0, 0, 0, 0.3);
        }

        button:active {
            transform: translateY(1px);
        }

        button:disabled {
            opacity: 0.7;
            transform: none;
            cursor: not-allowed;
            filter: grayscale(0.05);
        }

        .result-card {
            background: #f8fafc;
            border-radius: 1.5rem;
            padding: 1.2rem 1.2rem 1rem;
            margin-top: 2rem;
            border: 1px solid #e2edff;
            transition: 0.15s;
        }

        .result-header {
            display: flex;
            justify-content: space-between;
            align-items: baseline;
            flex-wrap: wrap;
            margin-bottom: 0.6rem;
        }

        .result-title {
            font-size: 0.7rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-weight: 700;
            color: #415a8c;
        }

        .copy-btn {
            background: none;
            border: none;
            font-size: 0.7rem;
            font-weight: 500;
            color: #6366f1;
            cursor: pointer;
            padding: 0.2rem 0.6rem;
            border-radius: 1rem;
            background: #eef2ff;
            width: auto;
            margin: 0;
            box-shadow: none;
            display: inline-flex;
            align-items: center;
            gap: 0.2rem;
        }

        .copy-btn:hover {
            background: #e0e7ff;
            transform: none;
            box-shadow: none;
        }

        .short-url {
            font-size: 1.15rem;
            font-weight: 600;
            word-break: break-all;
            margin: 0.2rem 0 0.6rem;
        }

        .short-url a {
            color: #2563eb;
            text-decoration: none;
            border-bottom: 1px dashed #94a3b8;
        }

        .short-url a:hover {
            color: #1d4ed8;
            border-bottom-style: solid;
        }

        .original-url {
            font-size: 0.8rem;
            color: #475569;
            word-break: break-all;
            background: #f1f5f9;
            padding: 0.5rem 0.8rem;
            border-radius: 1rem;
            margin: 0.7rem 0 0.3rem;
        }

        .qr-area {
            display: flex;
            justify-content: center;
            margin-top: 0.8rem;
            padding-top: 0.8rem;
            border-top: 1px solid #e2e8f0;
        }

        #qrcode {
            background: white;
            padding: 6px;
            border-radius: 12px;
            display: inline-block;
        }

        .error-msg {
            color: #dc2626;
            background: #fef2f2;
            border-radius: 1.2rem;
            padding: 0.7rem 1rem;
            font-size: 0.85rem;
            margin-top: 1rem;
            border-left: 3px solid #ef4444;
        }

        .footer-note {
            text-align: center;
            font-size: 0.7rem;
            color: #7c8aa5;
            margin-top: 1.6rem;
        }

        @media (max-width: 480px) {
            .card {
                padding: 1.5rem;
            }
            h1 {
                font-size: 1.5rem;
            }
            .short-url {
                font-size: 1rem;
            }
        }
    </style>
    <!-- 可选 QR 码库，轻量且可靠 -->
    <script src="https://cdn.jsdelivr.net/npm/qrcodejs@1.0.0/qrcode.min.js"></script>
</head>
<body>
<div class="card">
    <div style="display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap;">
        <h1>ShortLink <span class="badge">C++11</span></h1>
        <span style="font-size: 0.7rem; background: #eef2ff; padding: 0.2rem 0.6rem; border-radius: 30px;">⚡ Reactor</span>
    </div>
    <div class="sub">高性能网络库 · 毫秒级跳转</div>

    <div class="input-group">
        <label>🔗 长链接地址</label>
        <input type="url" id="longUrlInput" placeholder="https:// 或 http:// 链接" autofocus>
    </div>

    <button id="shortenBtn">✨ 生成短链接</button>

    <!-- 结果区域，初始隐藏 -->
    <div id="resultArea" style="display: none;">
        <div class="result-card">
            <div class="result-header">
                <span class="result-title">🔗 短链接 (点击可直接跳转)</span>
                <button class="copy-btn" id="copyShortBtn">📋 复制</button>
            </div>
            <div class="short-url" id="shortUrlDisplay"></div>
            <div class="original-url" id="originalUrlDisplay"></div>
            <div class="qr-area">
                <div id="qrcode"></div>
            </div>
            <div id="resultError" class="error-msg" style="display: none;"></div>
        </div>
    </div>
    <div id="globalError" class="error-msg" style="display: none;"></div>
    <div class="footer-note">
        支持 https/http · 永久有效 · 无广告
    </div>
</div>

<script>
    (function() {
        const inputEl = document.getElementById('longUrlInput');
        const btn = document.getElementById('shortenBtn');
        const resultDiv = document.getElementById('resultArea');
        const shortUrlSpan = document.getElementById('shortUrlDisplay');
        const originalSpan = document.getElementById('originalUrlDisplay');
        const globalErrorDiv = document.getElementById('globalError');
        const resultErrorDiv = document.getElementById('resultError');
        const copyBtn = document.getElementById('copyShortBtn');
        let currentShortUrl = '';

        // 清除之前的二维码 (若有残留)
        function clearQR() {
            const qrContainer = document.getElementById('qrcode');
            if (qrContainer) {
                qrContainer.innerHTML = '';
            }
        }

        // 显示结果区块中的数据
        function showResult(longUrl, code) {
            const origin = window.location.origin;
            const shortFull = origin + '/' + code;
            currentShortUrl = shortFull;

            // 显示短链接
            const shortLinkElem = document.createElement('a');
            shortLinkElem.href = shortFull;
            shortLinkElem.target = '_blank';
            shortLinkElem.textContent = shortFull;
            shortUrlSpan.innerHTML = '';
            shortUrlSpan.appendChild(shortLinkElem);

            originalSpan.textContent = longUrl;
            
            // 生成二维码
            clearQR();
            try {
                new QRCode(document.getElementById('qrcode'), {
                    text: shortFull,
                    width: 96,
                    height: 96,
                    colorDark: '#1f2937',
                    colorLight: '#ffffff',
                    correctLevel: QRCode.CorrectLevel.M
                });
            } catch(e) {
                console.warn("QR generation failed", e);
                const qrContainer = document.getElementById('qrcode');
                if (qrContainer) qrContainer.innerText = '⚡ 短链接已生成';
            }
            
            resultDiv.style.display = 'block';
            globalErrorDiv.style.display = 'none';
            resultErrorDiv.style.display = 'none';
        }

        function showGlobalError(msg) {
            globalErrorDiv.textContent = msg;
            globalErrorDiv.style.display = 'block';
            resultDiv.style.display = 'none';
            resultErrorDiv.style.display = 'none';
        }

        function showResultError(msg) {
            if (resultDiv.style.display === 'block') {
                resultErrorDiv.textContent = msg;
                resultErrorDiv.style.display = 'block';
            } else {
                showGlobalError(msg);
            }
        }

        async function shortenRequest() {
            let rawUrl = inputEl.value.trim();
            if (!rawUrl) {
                showGlobalError('请填写需要缩短的链接');
                return;
            }
            // 简单校验是否包含协议
            if (!rawUrl.startsWith('http://') && !rawUrl.startsWith('https://')) {
                rawUrl = 'https://' + rawUrl;
                inputEl.value = rawUrl;
            }
            
            btn.disabled = true;
            btn.textContent = '⏳ 生成中...';
            globalErrorDiv.style.display = 'none';
            resultDiv.style.display = 'none';
            
            try {
                const apiUrl = '/api/shorten?url=' + encodeURIComponent(rawUrl);
                const response = await fetch(apiUrl, { method: 'POST', headers: { 'Accept': 'application/json' } });
                const data = await response.json();
                
                if (response.ok && data && data.code) {
                    showResult(rawUrl, data.code);
                } else {
                    const errMsg = data && data.error ? data.error : '短链生成失败，请重试';
                    showResultError(errMsg);
                }
            } catch (err) {
                console.error(err);
                showResultError('网络错误或后端服务暂时不可用');
            } finally {
                btn.disabled = false;
                btn.textContent = '✨ 生成短链接';
            }
        }
        
        // 复制功能
        copyBtn.addEventListener('click', async () => {
            if (!currentShortUrl) return;
            try {
                await navigator.clipboard.writeText(currentShortUrl);
                const originalText = copyBtn.textContent;
                copyBtn.textContent = '✅ 已复制';
                setTimeout(() => {
                    copyBtn.textContent = originalText;
                }, 1500);
            } catch (e) {
                alert('无法复制，手动复制吧');
            }
        });
        
        btn.addEventListener('click', shortenRequest);
        inputEl.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                shortenRequest();
            }
        });
    })();
</script>
</body>
</html>
)raw";

#endif