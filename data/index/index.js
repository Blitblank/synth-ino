
let sliders = [];

let ws;
let reconnectInterval = 3000; // ms
let slidersUpdateInterval = 50;
let wsIndicator;

let canvas;
let ctx;

// create slider components out of divs
document.addEventListener("DOMContentLoaded", () => {

    document.querySelectorAll(".slider-block").forEach(el => {
        slider = createSliderComponent(el, {min:el.getAttribute("min"), max:el.getAttribute("max"), value:el.getAttribute("value"), step:el.getAttribute("step")});
        sliders.push(slider)
    });

    //wsIndicator = document.createElement("div");
    //wsIndicator.style.width = "16px";
    //wsIndicator.style.height = "16px";
    //wsIndicator.style.borderRadius = "50%";
    //wsIndicator.style.background = "red";
    //wsIndicator.style.display = "inline-block";
    //wsIndicator.style.marginLeft = "8px";
    //document.body.insertAdjacentHTML("afterbegin", "<p>WebSocket: </p>");
    //document.querySelector("p").appendChild(wsIndicator);

    initWebSocket();
    
    canvas = document.getElementById("scopeCanvas");
    initScope();

});

// TODO: move scope code into its own component
function initScope() {
    if (!canvas) {
        console.error('Scope canvas not found:', canvasId);
        return;
    }
    ctx = canvas.getContext('2d', { alpha: false });
    drawGrid(canvas.width, canvas.height);

    let sample = new Array(128);
    for(let i = 0; i < 128; i++) {
        sample[i] = 32;
    }
    drawWave(sample);
}

function drawGrid(w, h) {
    const bgColor = '#131313ff';
    const gridColor = '#1f242bff'
    const centerLineColor = '#4B4E6D';

    ctx.fillStyle = bgColor;
    ctx.fillRect(0, 0, w, h);

    ctx.strokeStyle = gridColor;
    ctx.lineWidth = 1;

    // vertical lines: 8 divisions
    const vDivs = 8;
    for (let i = 0; i <= vDivs; i++) {
        const x = Math.round(i * w / vDivs);
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h);
        ctx.stroke();
    }

    // horizontal center line
    ctx.strokeStyle = centerLineColor;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, h/2);
    ctx.lineTo(w, h/2);
    ctx.stroke();
}

function drawWave(samples) {
    const traceColor = '#DCF763';
    const w = canvas.width;
    const h = canvas.height;
    drawGrid(w, h);

    if (!samples) return;

    xStep = w/128;
    yStep = h/64;

    // draw trace
    ctx.lineWidth = 8;
    ctx.strokeStyle = traceColor;
    ctx.beginPath();
    for (let i = 0; i < samples.length; i++) {
        const x = Math.round(i * xStep);
        const y = Math.round(samples[i] * yStep);
        if (i === 0) {
            ctx.moveTo(x, y); 
        } else {
            ctx.lineTo(x, y);
        }
    }
    ctx.stroke();

}

// package config data for a message
function sendSliders() {

    let s = "";
    sliders.forEach(slider => {
        let comma = 
        s += slider.value + ","
    });
    s = s.slice(0, -1); // remove last comma

    let d = document.getElementById('d1').value + "," + document.getElementById('d2').value + "," + document.getElementById('d3').value + "," + "0";
    let payload = s + ";" + d;

    ws.send(payload);

}

let sendSliderscallbackId = 0;

function initWebSocket() {
    const protocol = location.protocol === "https:" ? "wss://" : "ws://";
    const url = protocol + location.host + "/ws";
    ws = new WebSocket(url);

    ws.binaryType = "arraybuffer";

    ws.onopen = () => {
        console.log("WebSocket connected");
        //wsIndicator.style.background = "limegreen";
        ws.send("hello from client");

        sendSliderscallbackId = setInterval(sendSliders, slidersUpdateInterval);
    };

    ws.onclose = (event) => {
        console.warn("WebSocket closed", event);
        //wsIndicator.style.background = "red";

        clearInterval(sendSliderscallbackId);

        setTimeout(initWebSocket, reconnectInterval); // attempt reconnect
    };

    ws.onerror = (err) => {
        console.error("WebSocket error", err);
        ws.close();
    };

    ws.onmessage = (msg) => {
        //console.log("WS message:", msg.data);

        // respond to ping from server
        if (msg.data === "ping") {
            ws.send("pong");
        }

        if (typeof msg.data === 'string') return;
        const buf = msg.data;
        const arr = new Uint8Array(buf);
        if (arr.length === 128) {
            drawWave(arr);
        } else {
            //console.log("bruger");
        }
    };
}

