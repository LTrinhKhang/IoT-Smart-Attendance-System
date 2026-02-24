const express = require('express');
const router = express.Router();
const os = require('os');

function getLocalIP() {
    const networkInterfaces = os.networkInterfaces();
    for (const key in networkInterfaces) {
        for (const net of networkInterfaces[key]) {
            if (net.family === 'IPv4' && !net.internal) {
                return net.address;
            }
        }
    }
    return '127.0.0.1';
}

router.get('/server-ip', (req, res) => {
    const ip = getLocalIP();
    res.json({ ip });
});

module.exports = router;
