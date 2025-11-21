async function setup() {
    const patchURL = "export/rasm_origin.json";

    const WA = window.AudioContext || window.webkitAudioContext;
    const context = new WA();

    const out = context.createGain();
    out.connect(context.destination);

    let patcher;
    try {
        const res = await fetch(patchURL);
        patcher = await res.json();
        if (!window.RNBO) {
            await loadRNBOScript(patcher.desc.meta.rnboversion);
        }
    } catch (e) {
        console.error(e);
        return;
    }

    let device;
    try {
        device = await RNBO.createDevice({ context, patcher });
        window.rnboDevice = device;
        window.rnboContext = context;
    } catch (e) {
        console.error(e);
        return;
    }

    device.node.connect(out);

    // Mic
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const mic = context.createMediaStreamSource(stream);
        mic.connect(device.node);
    } catch (e) {
        console.error(e);
    }

    // Resume on click
    document.body.onclick = () => context.resume();

    connectCustomSliders(device);
}


// -------------------------
// Load RNBO script
// -------------------------
function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        const el = document.createElement("script");
        el.src = `https://c74-public.nyc3.digitaloceanspaces.com/rnbo/${version}/rnbo.min.js`;
        el.onload = resolve;
        el.onerror = () => reject();
        document.body.append(el);
    });
}


// -------------------------
// Custom Slider Mapping
// -------------------------
function connectCustomSliders(device) {

    // RNBO parameters の仕様
    const specs = {
        volume:        { min: 0,  max: 1,  invert: false, step: 0.01 },
        sensitivity:   { min: 20, max: 80, invert: true,  step: 5    },
        dynamics:      { min: 0,  max: 4,  invert: false, step: 1    },
        responsiveness:{ min: 0,  max: 10, invert: true,  step: 1    },
        release:       { min: 0,  max: 10, invert: false, step: 1    }
    };

    const mapping = [
        { id: "volume-slider", param: "volume" },
        { id: "sensitivity-slider", param: "sensitivity" },
        { id: "dynamics-slider", param: "dynamics" },
        { id: "responsiveness-slider", param: "responsiveness" },
        { id: "release-slider", param: "release" }
    ];

    mapping.forEach(({ id, param }) => {

        const ui = document.getElementById(id);
        if (!ui) return;

        const p = device.parameters.find(x => x.name === param);
        if (!p) return;

        const spec = specs[param];

        // --------------------------
        // Param → Slider
        // --------------------------
        const paramToSlider = (v) => {
            const norm = (v - spec.min) / (spec.max - spec.min);
            const pos = norm * 100;
            return spec.invert ? (100 - pos) : pos;
        };

        // --------------------------
        // Slider → Param
        // --------------------------
        const sliderToParam = (v) => {
            let norm = spec.invert ? (100 - v) / 100 : v / 100;

            let raw = spec.min + norm * (spec.max - spec.min);

            if (spec.step > 0) {
                raw = Math.round(raw / spec.step) * spec.step;
            }

            return Math.min(spec.max, Math.max(spec.min, raw));
        };

        // 初期位置
        ui.value = paramToSlider(p.value);

        // UI → RNBO
        ui.addEventListener("input", (e) => {
            const val = sliderToParam(parseFloat(e.target.value));
            p.value = val;
        });

        // RNBO → UI
        device.parameterChangeEvent.subscribe(ev => {
            if (ev.id !== p.id) return;
            ui.value = paramToSlider(ev.value);
        });
    });
}

setup();

document.querySelectorAll(".slider-ticks").forEach(ticks => {
    const steps = Number(ticks.dataset.steps);
    ticks.style.setProperty("--steps", steps);
});
