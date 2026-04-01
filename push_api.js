// Push AnyClaw_LVGL to GitHub via Git Data API
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const TOKEN = process.env.GITHUB_TOKEN;
const OWNER = 'IwanFlag';
const REPO = 'AnyClaw_LVGL';
const BRANCH = 'main';
const BASE = 'C:\\Users\\thundersoft\\.openclaw\\workspace\\AnyClaw_LVGL';
const SKIP = ['.git', 'build', 'thirdparty\\lvgl\\src', 'thirdparty\\lvgl\\docs', 'thirdparty\\lvgl\\examples', 'thirdparty\\lvgl\\demos'];
const API = `https://api.github.com/repos/${OWNER}/${REPO}`;

function gh(method, url, body) {
    const tmp = `/tmp/gh_body_${process.pid}.json`;
    if (body) fs.writeFileSync(tmp, JSON.stringify(body));
    const cmd = body
        ? `curl.exe -s -X ${method} -H "Authorization: token ${TOKEN}" -H "Accept: application/vnd.github.v3+json" -H "Content-Type: application/json" -d "@${tmp}" "${url}"`
        : `curl.exe -s -X ${method} -H "Authorization: token ${TOKEN}" -H "Accept: application/vnd.github.v3+json" "${url}"`;
    const result = execSync(cmd, { maxBuffer: 10 * 1024 * 1024 }).toString().trim();
    if (body) fs.unlinkSync(tmp);
    return JSON.parse(result);
}

// Get HEAD
const refData = gh('GET', `${API}/git/ref/heads/${BRANCH}`);
const headSha = refData.object.sha;
console.log('HEAD:', headSha);

// Collect files
function collectFiles(dir) {
    const files = [];
    function walk(d) {
        const entries = fs.readdirSync(d, { withFileTypes: true });
        for (const e of entries) {
            const full = path.join(d, e.name);
            if (e.isDirectory()) {
                let skip = false;
                for (const s of SKIP) {
                    if (full.replace(/\\/g, '/').includes(s.replace(/\\/g, '/'))) { skip = true; break }
                }
                if (!skip) walk(full);
            } else if (e.isFile()) {
                const stat = fs.statSync(full);
                if (stat.size < 1024 * 1024) { // < 1MB
                    const rel = full.substring(BASE.length + 1).replace(/\\/g, '/');
                    files.push({ rel, full });
                }
            }
        }
    }
    walk(dir);
    return files;
}

console.log('Collecting files...');
const files = collectFiles(BASE);
console.log('Files:', files.length);

// Create blobs
console.log('Creating blobs...');
const blobMap = {};
let ok = 0;
for (let i = 0; i < files.length; i++) {
    const content = fs.readFileSync(files[i].full, 'utf8');
    try {
        const r = gh('POST', `${API}/git/blobs`, { content: Buffer.from(content).toString('base64'), encoding: 'base64' });
        blobMap[files[i].rel] = r.sha;
        ok++;
    } catch (e) {
        console.error('  FAIL:', files[i].rel);
    }
    if ((i + 1) % 200 === 0 || i === files.length - 1) {
        console.log(`  ${i + 1}/${files.length} [ok=${ok}]`);
    }
}

// Create tree
console.log('Creating tree...');
const treeEntries = Object.entries(blobMap).map(([path, sha]) => ({ path, mode: '100644', type: 'blob', sha }));
const treeResult = gh('POST', `${API}/git/trees`, { base_tree: headSha, tree: treeEntries });
console.log('Tree:', treeResult.sha);

// Create commit
console.log('Creating commit...');
const commitResult = gh('POST', `${API}/git/commits`, {
    message: 'fix: maximize/restore, window size, garbled text',
    tree: treeResult.sha,
    parents: [headSha]
});
console.log('Commit:', commitResult.sha);

// Update ref
console.log('Updating ref...');
gh('PATCH', `${API}/git/refs/heads/${BRANCH}`, { sha: commitResult.sha });
console.log(`Done! Pushed ${ok} files.`);
