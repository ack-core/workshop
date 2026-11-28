// Runs editor in browser: node runner.js

const https = require('https');
const fs = require('fs');
const path = require('path');

module.paths.push(require('child_process').execSync('npm root -g').toString().trim());
const { chromium } = require('playwright');

const MIME = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.css': 'text/css', '.svg': 'image/svg+xml' };
const ROOT_URL = '/fs/';

// Stand-in for FileSystemDirectoryHandle on repo root, files are accessed via ROOT_URL
function standInFileSystem(rootUrl) {
    const request = async (url, method, body) => {
        const response = await fetch(url, { method, body });
        if (response.ok == false) {
            throw new DOMException(url, 'NotFoundError');
        }
        return response;
    };

    class FileHandle {
        constructor(url, name) { Object.assign(this, { kind: 'file', url, name }); }
        async getFile() { return new File([await request(this.url, 'GET').then(r => r.blob())], this.name); }
        async createWritable() {
            const chunks = [];
            return { write: async data => chunks.push(data), close: () => request(this.url, 'PUT', new Blob(chunks)) };
        }
        remove() { return request(this.url, 'DELETE'); }
    }

    class DirectoryHandle {
        constructor(url, name) { Object.assign(this, { kind: 'directory', url, name }); }
        async queryPermission() { return 'granted'; }
        async getDirectoryHandle(name, options) {
            await request(this.url + name + '/', options?.create ? 'POST' : 'HEAD');
            return new DirectoryHandle(this.url + name + '/', name);
        }
        async getFileHandle(name, options) {
            if (options?.create != true) {
                await request(this.url + name, 'HEAD');
            }
            return new FileHandle(this.url + name, name);
        }
        async *values() {
            for (const { name, kind } of await request(this.url, 'GET').then(r => r.json())) {
                yield kind == 'directory' ? new DirectoryHandle(this.url + name + '/', name) : new FileHandle(this.url + name, name);
            }
        }
    }

    const root = new DirectoryHandle(rootUrl, 'root');
    const get = IDBObjectStore.prototype.get;

    window.showDirectoryPicker = async () => root;
    window.showSaveFilePicker = async options => options.startIn.getFileHandle(options.suggestedName + Object.values(options.types[0].accept)[0][0], { create: true });
		
    IDBObjectStore.prototype.get = function (key) {
        const result = get.call(this, key);
        if (key == 'rootDirectory') {
            Object.defineProperty(result, 'result', { value: root });
        }
        return result;
    };
}

https.createServer({
    cert: fs.readFileSync(path.join(__dirname, 'cert.pem')),
    key: fs.readFileSync(path.join(__dirname, 'key.pem')),
}, (req, res) => {
    if (req.url == '/host_cmd_update') {
        require('child_process').execFileSync('python', [path.join(__dirname, 'update_resources.py')], { stdio: 'inherit' });
        res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
        return res.end('done');
    }

    const url = decodeURIComponent(req.url);
    const filePath = url.startsWith(ROOT_URL) ? path.join(__dirname, '..', url.slice(ROOT_URL.length)) : path.join(__dirname, url);
    const reply = (err, data) => {
        res.writeHead(err ? 404 : 200);
        res.end(data);
    };

    if (url.startsWith(ROOT_URL) == false && filePath.startsWith(path.join(__dirname, 'data')) == false && req.method != 'GET') {
        res.writeHead(403);
        return res.end();
    }
    if (req.method == 'PUT') {
        const chunks = [];
        req.on('data', chunk => chunks.push(chunk));
        req.on('end', () => {
            fs.mkdirSync(path.dirname(filePath), { recursive: true });
            fs.writeFile(filePath, Buffer.concat(chunks), reply);
        });
        return;
    }
    if (req.method == 'DELETE') {
        return fs.rm(filePath, { force: true }, reply);
    }
    if (req.method == 'POST') {
        return fs.mkdir(filePath, { recursive: true }, err => reply(err));
    }
    if (req.method == 'HEAD') {
        return fs.access(filePath, reply);
    }
    if (fs.statSync(filePath, { throwIfNoEntry: false })?.isDirectory()) {
        return fs.readdir(filePath, { withFileTypes: true }, (err, entries) => {
            reply(err, JSON.stringify(entries?.map(e => ({ name: e.name, kind: e.isDirectory() ? 'directory' : 'file' }))));
        });
    }

    fs.readFile(filePath, (err, data) => {
        res.writeHead(err ? 404 : 200, {
            'Content-Type': MIME[path.extname(url)] || 'application/octet-stream',
            'Cross-Origin-Embedder-Policy': 'credentialless',
            'Cross-Origin-Opener-Policy': 'same-origin',
        });
        res.end(data);
    });
}).listen(9002, async () => {
    const ci = process.env.CI != null;
    const browser = await chromium.launch({ headless: ci, args: ci ? ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] : [] });
    const page = await browser.newPage({ ignoreHTTPSErrors: true, viewport: { width: 1024, height: 1024 } });

    page.on('close', () => process.exit(0));
    page.on('console', msg => console.log(msg.text()));
    page.on('pageerror', err => {
        console.log(err.message);
        process.exit(1);
    });

    await page.addInitScript(standInFileSystem, ROOT_URL);

    for (const [name, scenario] of Object.entries(require('./scenarios.js'))) {
        await page.goto('https://127.0.0.1:9002/editor.html');
        await page.waitForFunction(() => testing_visible('top_buttons') && uiEditor['engine.refresh'] != null);
        console.log("--- Scenario started: " + name + " ---");
        await scenario(page);
        await page.screenshot({ path: path.join(__dirname, '..', 'screens', 'editor', name + '.png') });
    }

	console.log("--- Tests finished ---");
    console.log("Editor screenshots have been saved into 'screens/editor'");
    await page.waitForTimeout(2000);
    process.exit(0);

});
