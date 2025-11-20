/* ----------------------------------------------------------
   RASM origin – Ultra Minimal RNBO App.js
   (Inport / Outport / Preset / MIDI Keyboard 完全削除)
   ---------------------------------------------------------- */

async function setup() {

    const patchExportURL = "export/rasm_origin.json";

    // --- AudioContext 生成 ---
    const WAContext = window.AudioContext || window.webkitAudioContext;
    const context = new WAContext();

    // 出力ノード
    const outputNode = context.createGain();
    outputNode.connect(context.destination);

    // --- patcher 読み込み ---
    let response, patcher;
    try {
        response = await fetch(patchExportURL);
        patcher = await response.json();

        if (!window.RNBO) {
            await loadRNBOScript(patcher.desc.meta.rnboversion);
        }

    } catch (err) {
        console.error("Failed to load patcher:", err);
        return;
    }

    // --- Dependencies 読み込み ---
    let dependencies = [];
    try {
        const depRes = await fetch("export/dependencies.json");
        dependencies = await depRes.json();
        dependencies = dependencies.map(d =>
            d.file ? { ...d, file: "export/" + d.file } : d
        );
    } catch (e) {}


    // --- Device 作成 ---
    let device;
    try {
        device = await RNBO.createDevice({ context, patcher });
        window.rnboDevice = device;
        window.rnboContext = context;
    } catch (err) {
        console.error("Device creation failed:", err);
        return;
    }

    if (dependencies.length) {
        await device.loadDataBufferDependencies(dependencies);
    }

    // AudioWorklet → 出力へ
    device.node.connect(outputNode);


    /* ----------------------------------------------------------
       ★ ここから：あなたのHTML UI 専用のスライダー接続
       ---------------------------------------------------------- */

    const parameterRanges = {
        volume:        { min: 0,  max: 1  },
        sensitivity:   { min: 20, max: 80 },
        responsiveness:{ min: 0,  max: 10 },
        dynamics:      { min: 0,  max: 4  },
        release:       { min: 0,  max: 10 }
    };

    const stepTable = {
        volume: 0.01,
        sensitivity: 5,       // 12段階
        responsiveness: 1,    // 10段階
        dynamics: 1,          // 4段階
        release: 1            // 10段階
    };

    const reversedParams = ["sensitivity", "responsiveness"];

    const sliderMap = [
        { id: "volume-slider",          param: "volume" },
        { id: "sensitivity-slider",     param: "sensitivity" },
        { id: "dynamics-slider",        param: "dynamics" },
        { id: "responsiveness-slider",  param: "responsiveness" },
        { id: "release-slider",         param: "release" }
    ];

    sliderMap.forEach(({ id, param }) => {

        const slider = document.getElementById(id);
        if (!slider) return;

        const rnboParam =
            device.parameters.find(p => p.name === param) ||
            device.parameters.find(p => p.id === param);

        if (!rnboParam) return;

        const range = parameterRanges[param];
        const step = stepTable[param];

        const isReversed = reversedParams.includes(param);

        // RNBO → UI
        const paramToSlider = (value) => {
            const norm = (value - range.min) / (range.max - range.min);
            return isReversed ? (100 - norm * 100) : (norm * 100);
        };

        // UI → RNBO + step 丸め
        const sliderToParam = (sliderValue) => {
            const raw = isReversed
                ? range.min + ((100 - sliderValue) / 100) * (range.max - range.min)
                : range.min + (sliderValue / 100) * (range.max - range.min);

            return Math.round(raw / step) * step;
        };

        // 初期値反映
        slider.value = paramToSlider(rnboParam.value);

        // ユーザー操作
        slider.addEventListener("input", (e) => {
            const v = sliderToParam(parseFloat(e.target.value));
            rnboParam.value = v;
        });

        // RNBO → UI（値の同期）
        device.parameterChangeEvent.subscribe((ev) => {
            if (ev.id !== rnboParam.id) return;
            slider.value = paramToSlider(ev.value);
        });
    });

    /* ----------------------------------------------------------
       ここまで：UI スライダー反映
       ---------------------------------------------------------- */


    // --- マイク入力 ---
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const mic = context.createMediaStreamSource(stream);
        mic.connect(device.node);
        console.log("Mic OK");
    } catch (err) {
        console.error("Microphone error:", err);
    }

    // --- クリックで resume ---
    document.body.onclick = () => context.resume();
}


/* RNBO runtime loader */
function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        const el = document.createElement("script");
        el.src =
            "https://c74-public.nyc3.digitaloceanspaces.com/rnbo/" +
            encodeURIComponent(version) +
            "/rnbo.min.js";
        el.onload = resolve;
        el.onerror = () => reject("Failed to load RNBO runtime");
        document.body.append(el);
    });
}

setup();
