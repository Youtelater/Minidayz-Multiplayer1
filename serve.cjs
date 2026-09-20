// Serve this folder so two browser tabs can play against each other.
//
//     node serve.cjs            -> http://127.0.0.1:8777
//     node serve.cjs 9000       -> a different port
//
// Open the URL in two separate windows (or one normal + one private window, so
// they get separate localStorage and count as two different players). Host in
// one, join with the room id in the other.
const fs = require('fs');
const path = require('path');
const http = require('http');

const root = __dirname;
const port = Number(process.argv[2]) || 8777;
const mime = {
    '.js': 'application/javascript', '.html': 'text/html', '.css': 'text/css',
    '.png': 'image/png', '.jpg': 'image/jpeg', '.jpeg': 'image/jpeg', '.gif': 'image/gif',
    '.xml': 'text/xml', '.json': 'application/json', '.webmanifest': 'application/manifest+json',
    '.ogg': 'audio/ogg', '.m4a': 'audio/mp4', '.mp3': 'audio/mpeg', '.wav': 'audio/wav',
    '.woff': 'font/woff', '.woff2': 'font/woff2', '.ttf': 'font/ttf', '.svg': 'image/svg+xml'
};

http.createServer(function (req, res) {
    var rel;
    try {
        rel = decodeURIComponent(new URL(req.url, 'http://localhost').pathname).replace(/^\/+/, '');
    } catch (e) { res.writeHead(400); return res.end(); }
    if (!rel) rel = 'index.html';

    var file = path.resolve(root, rel);
    // never serve outside this folder
    if (file !== root && !file.startsWith(root + path.sep)) { res.writeHead(403); return res.end(); }

    fs.readFile(file, function (err, body) {
        if (err) { res.writeHead(404); return res.end('not found: ' + rel); }
        res.writeHead(200, {
            'Content-Type': mime[path.extname(file).toLowerCase()] || 'application/octet-stream',
            'Cache-Control': 'no-store'
        });
        res.end(body);
    });
}).listen(port, '127.0.0.1', function () {
    console.log('MiniDayZ+ 2.3.3 multiplayer served at http://127.0.0.1:' + port);
    console.log('Open it in TWO windows - use a private window for the second player,');
    console.log('so each gets its own storage and they count as two players.');
    console.log('Ctrl+C to stop.');
});
