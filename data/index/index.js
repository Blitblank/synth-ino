
let sliders = [];

// create slider components out of divs
document.addEventListener("DOMContentLoaded", () => {

    document.querySelectorAll(".slider-block").forEach(el => {
        slider = createSliderComponent(el, {min:el.getAttribute("min"), max:el.getAttribute("max"), value:el.getAttribute("value"), step:el.getAttribute("step")});
        sliders.push(slider)
    });

});

let ws = new WebSocket('ws://' + location.host + '/ws');

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

setInterval(sendSliders, 100);
// TODO: if websocket is closed, send a request to re-open
