
class SmartSlider extends HTMLElement {
    static get observedAttributes() {
        return ["min", "max", "step", "value", "label"];
    }

    constructor() {
        super();

        // Template
        const template = document.createElement("template");
        template.innerHTML = `
            <div class="slider-label"></div>
            <input id="minInput" class="side-input" type="text" />
            <input id="slider" type="range" />
            <input id="maxInput" class="side-input" type="text" />
            <input id="stepInput" type="text" />
            <input id="valueInput" type="text" />
        `;

        this.appendChild(template.content.cloneNode(true));

        this.labelEl = this.querySelector(".slider-label");
        this.slider = this.querySelector("#slider");
        this.minInput = this.querySelector("#minInput");
        this.maxInput = this.querySelector("#maxInput");
        this.stepInput = this.querySelector("#stepInput");
        this.valueInput = this.querySelector("#valueInput");

        const updateRange = () => {
            const min = parseFloat(this.minInput.value) || this.slider.min;
            const max = parseFloat(this.maxInput.value) || this.slider.max;
            const stepVal = parseFloat(this.stepInput.value) || (max - min) / 100;
            this.slider.min = min;
            this.slider.max = max;
            this.slider.step = stepVal;
        };

        this.slider.addEventListener("input", () => this.value = this.slider.value);
        this.minInput.addEventListener("change", updateRange);
        this.maxInput.addEventListener("change", updateRange);
        this.stepInput.addEventListener("change", updateRange);
        this.valueInput.addEventListener("keydown", (ev) => {
        if (ev.key === "Enter") {
            ev.preventDefault();
            const val = parseFloat(this.valueInput.value);
            if (!isNaN(val)) {
            this.value = val;
            this.slider.value = val;
            }
        }
        });

        // text box reacts to slider position
        const observer = new MutationObserver(() => {
            this.valueInput.value = this.slider.value;
        });
        observer.observe(this.slider, { attributes: true, attributeFilter: ["value"] });

        // actual constructor
        this.updateFromAttributes();
        updateRange();
    }

    connectedCallback() {
        this.updateFromAttributes();
    }

    updateFromAttributes() {
        const min = this.getAttribute("min") ?? "0";
        const max = this.getAttribute("max") ?? "100";
        const step = this.getAttribute("step") ?? "1";
        const value = this.getAttribute("value") ?? (parseFloat(min) + parseFloat(max)) / 2;
        const label = this.getAttribute("label") ?? "";

        this.minInput.value = min;
        this.maxInput.value = max;
        this.stepInput.value = step;
        this.valueInput.value = value;
        this.labelEl.textContent = label;

        this.slider.min = min;
        this.slider.max = max;
        this.slider.step = step;
        this.slider.value = value;
    }

    // Property accessors
    get value() { return this.slider.value; }
    set value(v) {
        this.setAttribute("value", v);
        this.slider.value = v;
        this.valueInput.value = v;
    }

    get min() { return this.slider.min; }
    set min(v) { this.setAttribute("min", v); }

    get max() { return this.slider.max; }
    set max(v) { this.setAttribute("max", v); }

    get step() { return this.slider.step; }
    set step(v) { this.setAttribute("step", v); }

    get label() { return this.labelEl.textContent; }
    set label(v) { this.setAttribute("label", v); }
}

customElements.define("smart-slider", SmartSlider);
