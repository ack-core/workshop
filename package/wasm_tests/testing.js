// Runs wasm tests in headless browser: node testing.js
// Exit code: 0 - all tests passed, 1 - failed

const https = require('https');
const fs = require('fs');
const path = require('path');

module.paths.push(require('child_process').execSync('npm root -g').toString().trim());
const { chromium } = require('playwright');

const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };

https.createServer({
    cert: fs.readFileSync(path.join(__dirname, 'cert.pem')),
    key: fs.readFileSync(path.join(__dirname, 'key.pem')),
}, (req, res) => {
    fs.readFile(path.join(__dirname, req.url), (err, data) => {
        res.writeHead(err ? 404 : 200, {
            'Content-Type': MIME[path.extname(req.url)] || 'application/octet-stream',
            'Cross-Origin-Embedder-Policy': 'credentialless',
            'Cross-Origin-Opener-Policy': 'same-origin',
        });
        res.end(data);
    });
}).listen(9002, async () => {
    const browser = await chromium.launch();
    const page = await browser.newPage({ ignoreHTTPSErrors: true });

    page.on('console', msg => console.log(msg.text()));
    page.on('pageerror', err => console.log(err.message));

    await page.goto('https://127.0.0.1:9002/index.html');
    const request = await page.waitForRequest(/\/(success|fail)$/, { timeout: 0 });
    process.exit(request.url().endsWith('/success') ? 0 : 1);
});
