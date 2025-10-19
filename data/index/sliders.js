
function createSliderComponent(container, opts) {
    const min = opts.min ?? 0;
    const max = opts.max ?? 100;
    const value = opts.value ?? (min + max) / 2;
    const step = opts.step ?? 1;
    const label = container.getAttribute("label");

    container.innerHTML = `
        <div class="slider-label">${label}</div>

        <input id="minInput" class="side-input" type="text" value="${min}" />
        <input id="slider" type="range" step="${step}"/>
        <input id="maxInput" class="side-input" type="text" value="${max}" />

        <input id="stepInput" type="text" value="${step}" />
        <input id="valueInput" type="text" value="${value}" />
    `;

    const slider = container.querySelector("#slider");
    const minInput = container.querySelector("#minInput");
    const maxInput = container.querySelector("#maxInput");
    const stepInput = container.querySelector("#stepInput");
    const valueInput  = container.querySelector("#valueInput");

    function updateRange() {
        const min = parseFloat(minInput.value) || 0;
        const max = parseFloat(maxInput.value) || 100;
        const stepVal = parseFloat(stepInput.value) || (max-min)/100;

        slider.min = min;
        slider.max = max;
        slider.step = stepVal;
    }

    updateRange();
    slider.value = value;
    valueInput.value = value;

    slider.addEventListener("input", () => valueInput.value = slider.value);
    minInput.addEventListener("change", updateRange);
    maxInput.addEventListener("change", updateRange);
    stepInput.addEventListener("change", updateRange);
    valueInput.addEventListener('keydown', (ev) => {
        if (ev.key === 'Enter') {
            ev.preventDefault();
            const val = parseFloat(v);
            slider.value = String(val);
            valueInput.value = val;
        }
    });
    
    // observer updates ui if the values are changes elsewhere
    const observer = new MutationObserver(() => {
        valueInput.value = formatNumber(Number(slider.value));
    });
    observer.observe(slider, { attributes: true, attributeFilter: ['value'] });

    return slider;
}
