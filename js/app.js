// =====================================
// RNBO Web Audio Device Setup
// =====================================

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

    // Mic input
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const mic = context.createMediaStreamSource(stream);
        mic.connect(device.node);
    } catch (e) {
        console.error("Microphone error:", e);
    }

    // Resume on click
    document.body.onclick = () => context.resume();

    // 呼び出し順
    connectCustomSliders(device);  // RNBO param <-> Slider
    generateAllTicks();            // DOM ticks を全スライダーへ
}



// =====================================
// RNBO script loader
// =====================================
function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        const el = document.createElement("script");
        el.src = `https://c74-public.nyc3.digitaloceanspaces.com/rnbo/${version}/rnbo.min.js`;
        el.onload = resolve;
        el.onerror = () => reject(new Error("Failed to load rnbo.js"));
        document.body.append(el);
    });
}



// =====================================
// DOM ticks generator（完全版）
// =====================================
// steps = 区間数 → ticks = steps + 1
// 例：12 steps → 13 ticks
function generateTicksFor(container, steps, slider) {
    container.innerHTML = ""; // 初期化

    const count = steps + 1;

    for (let i = 0; i < count; i++) {
        const t = document.createElement("div");
        t.className = "tick";

        // 位置：スライダーの 0 → 100 に対して線の中心を対応
        const pos = (i / steps) * 100;
        t.style.left = `calc(${pos}% )`;

        container.appendChild(t);
    }
}


// 全スライダーに ticks を生成
function generateAllTicks() {
    document.querySelectorAll(".slider-ticks").forEach(ticks => {
        const steps = Number(ticks.dataset.steps);
        const slider = ticks.parentElement.querySelector(".horizontal-slider");
        if (steps >= 1 && slider) {
            // CSS変数 --steps を設定
            ticks.style.setProperty('--steps', steps);
            generateTicksFor(ticks, steps, slider);
        }
    });
}



// =====================================
// RNBO <-> Custom Slider Mapping
// =====================================
function connectCustomSliders(device) {

    // param spec
    const specs = {
        volume:         { min: 0,  max: 1,  invert: false, step: 0.01 },

        sensitivity:    { min: 20, max: 80, invert: true,  step: 5    }, // 20〜80, 12区間
        dynamics:       { min: 0,  max: 4,  invert: false, step: 1    }, // 0〜4
        responsiveness: { min: 0,  max: 10, invert: true,  step: 1    },
        release:        { min: 0,  max: 10, invert: false, step: 1    }
    };

    const mapping = [
        { ui: "volume-slider",         param: "volume" },
        { ui: "sensitivity-slider",    param: "sensitivity" },
        { ui: "dynamics-slider",       param: "dynamics" },
        { ui: "responsiveness-slider", param: "responsiveness" },
        { ui: "release-slider",        param: "release" }
    ];

    mapping.forEach(({ ui, param }) => {
        const slider = document.getElementById(ui);
        const p = device.parameters.find(x => x.name === param);
        if (!slider || !p) return;

        const spec = specs[param];

        // 0〜100 の slider 空間におけるステップ幅
        if (spec.step > 0) {
            const intervals = (spec.max - spec.min) / spec.step;
            slider.step = 100 / intervals;
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
            raw = Math.round(raw / spec.step) * spec.step; // スナップ
            return Math.min(spec.max, Math.max(spec.min, raw));
        };

        // 初期値反映：RNBO の値を UI に
        slider.value = paramToSlider(p.value);

        // UI → RNBO
        slider.addEventListener("input", (e) => {
            p.value = sliderToParam(parseFloat(e.target.value));
        });

        // RNBO → UI（外部から更新されたら追随）
        device.parameterChangeEvent.subscribe(ev => {
            if (ev.id === p.id) {
                slider.value = paramToSlider(ev.value);
            }
        });
    });
}



// =====================================
setup();
