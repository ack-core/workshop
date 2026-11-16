// Runs editor in browser: node testing.js

const https = require('https');
const fs = require('fs');
const path = require('path');

module.paths.push(require('child_process').execSync('npm root -g').toString().trim());
const { chromium } = require('playwright');

const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.css': 'text/css', '.svg': 'image/svg+xml' };

https.createServer({
    cert: fs.readFileSync(path.join(__dirname, 'cert.pem')),
    key: fs.readFileSync(path.join(__dirname, 'key.pem')),
}, (req, res) => {
    const filePath = path.join(__dirname, decodeURIComponent(req.url));

    if (req.url == '/host_cmd_update') {
        require('child_process').execFileSync('python', [path.join(__dirname, 'update_resources.py')], { stdio: 'inherit' });
        res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
        return res.end('done');
    }

    if (filePath.startsWith(path.join(__dirname, 'data')) == false && req.method != 'GET') {
        res.writeHead(403);
        return res.end();
    }
    if (req.method == 'PUT') {
        const chunks = [];
        req.on('data', chunk => chunks.push(chunk));
        req.on('end', () => {
            fs.mkdirSync(path.dirname(filePath), { recursive: true });
            fs.writeFile(filePath, Buffer.concat(chunks), err => {
                res.writeHead(err ? 500 : 200);
                res.end();
            });
        });
        return;
    }
    if (req.method == 'DELETE') {
        return fs.rm(filePath, { force: true }, err => {
            res.writeHead(err ? 500 : 200);
            res.end();
        });
    }

    fs.readFile(filePath, (err, data) => {
        res.writeHead(err ? 404 : 200, {
            'Content-Type': MIME[path.extname(req.url)] || 'application/octet-stream',
            'Cross-Origin-Embedder-Policy': 'credentialless',
            'Cross-Origin-Opener-Policy': 'same-origin',
        });
        res.end(data);
    });
}).listen(9002, async () => {
    const browser = await chromium.launch({ headless: false });
    const page = await browser.newPage({ ignoreHTTPSErrors: true, viewport: null });

    page.on('close', () => process.exit(0));
    page.on('console', msg => console.log(msg.text()));
    page.on('pageerror', err => {
        console.log(err.message);
        process.exit(1);
    });

    await page.goto('https://127.0.0.1:9002/editor.html');
});
