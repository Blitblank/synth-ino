
let sliders = [];

let ws;
let reconnectInterval = 3000; // ms
let wsIndicator;

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
});

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

    ws.onopen = () => {
        console.log("WebSocket connected");
        //wsIndicator.style.background = "limegreen";
        ws.send("hello from client");

        sendSliderscallbackId = setInterval(sendSliders, 100);
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
        console.log("WS message:", msg.data);

        // respond to ping from server
        if (msg.data === "ping") {
            ws.send("pong");
        }
    };
}

