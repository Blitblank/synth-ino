
let ws = new WebSocket('ws://' + location.host + '/ws');
function sendSliders() {
    let s = [1,2,3,4,5].map(i => document.getElementById('s'+i).value).join(',');
    let d = [1,2,3,4].map(i => document.getElementById('d'+i).value).join(',');
    ws.send(s + ';' + d);
}
setInterval(sendSliders, 100);

// TODO: attempt to reconnect on websocket disconnect
// TODO: add an indicator for websocket connectivity