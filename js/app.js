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
        console.error("Failed to load patcher:", e);
        return;
    }

    let device;
    try {
        device = await RNBO.createDevice({ context, patcher });
        window.rnboDevice = device;
        window.rnboContext = context;
    } catch (e) {
        console.error("Failed to create RNBO device:", e);
        return;
    }

    device.node.connect(out);

    // Mic
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const mic = context.createMediaStreamSource(stream);
        mic.connect(device.node);
    } catch (e) {
        console.error("Microphone error:", e);
    }

    // Resume on click
    document.body.onclick = () => context.resume();

    // UI と RNBO を接続
    connectCustomSliders(device);

    // Ticks 用 CSS 変数を data-steps から注入
    initSliderTicks();
}


// -------------------------
// RNBO script loader
// -------------------------
function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        const el = document.createElement("script");
        el.src = `https://c74-public.nyc3.digitaloceanspaces.com/rnbo/${version}/rnbo.min.js`;
        el.onload = resolve;
        el.onerror = () => reject(new Error("Failed to load rnbo.js"));
        document.body.append(el);
    });
}


// -------------------------
// Ticks 初期化（CSS変数 --steps をセット）
// -------------------------
function initSliderTicks() {
    document.querySelectorAll(".slider-ticks").forEach(ticks => {
        const steps = Number(ticks.dataset.steps);
        if (!isNaN(steps) && steps > 0) {
            // 区間数 = steps → CSS 側は var(--steps) を使う
            ticks.style.setProperty("--steps", steps);
        }
    });
}


// -------------------------
// Custom Slider ↔ RNBO mapping
// -------------------------
function connectCustomSliders(device) {

    // RNBO パラメータ仕様
    const specs = {
        volume:         { min: 0,  max: 1,  invert: false, step: 0.01 },
        sensitivity:    { min: 20, max: 80, invert: true,  step: 5    }, // 20–80, 5刻み（12区間）
        dynamics:       { min: 0,  max: 4,  invert: false, step: 1    }, // 0–4, 1刻み（4区間）
        responsiveness: { min: 0,  max: 10, invert: true,  step: 1    }, // 0–10, 1刻み（10区間）
        release:        { min: 0,  max: 10, invert: false, step: 1    }
    };

    const mapping = [
        { id: "volume-slider",         param: "volume" },
        { id: "sensitivity-slider",    param: "sensitivity" },
        { id: "dynamics-slider",       param: "dynamics" },
        { id: "responsiveness-slider", param: "responsiveness" },
        { id: "release-slider",        param: "release" }
    ];

    mapping.forEach(({ id, param }) => {
        const ui = document.getElementById(id);
        if (!ui) return;

        const p = device.parameters.find(x => x.name === param);
        if (!p) return;

        const spec = specs[param];

        // 0–100 の slider 空間におけるステップ幅
        // → (param.max - param.min) を spec.step で割った区間数に対応
        if (spec.step > 0 && spec.max > spec.min) {
            const sliderStep = 100 * spec.step / (spec.max - spec.min);
            ui.step = sliderStep; // これで UI も飛び飛びにスナップ
        }

        // Param → Slider
        const paramToSlider = (v) => {
            const norm = (v - spec.min) / (spec.max - spec.min);
            const pos = norm * 100;
            return spec.invert ? (100 - pos) : pos;
        };

        // Slider → Param
        const sliderToParam = (v) => {
            let norm = spec.invert ? (100 - v) / 100 : v / 100;
            let raw = spec.min + norm * (spec.max - spec.min);

            if (spec.step > 0) {
                raw = Math.round(raw / spec.step) * spec.step;
            }

            return Math.min(spec.max, Math.max(spec.min, raw));
        };

        // 初期位置：RNBO の現在値を UI に反映（初期値は RNBO 側を優先）
        ui.value = paramToSlider(p.value);

        // UI → RNBO
        ui.addEventListener("input", (e) => {
            const val = sliderToParam(parseFloat(e.target.value));
            p.value = val;
        });

        // RNBO → UI（外部から param が変わったときも追従）
        device.parameterChangeEvent.subscribe(ev => {
            if (ev.id !== p.id) return;
            ui.value = paramToSlider(ev.value);
        });
    });
}

setup();
