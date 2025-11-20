async function setup() {
    const patchExportURL = "export/rasm_origin.json";

    const WAContext = window.AudioContext || window.webkitAudioContext;
    const context = new WAContext();

    const outputNode = context.createGain();
    outputNode.connect(context.destination);
    
    let response, patcher;
    try {
        response = await fetch(patchExportURL);
        patcher = await response.json();
    
        if (!window.RNBO) {
            await loadRNBOScript(patcher.desc.meta.rnboversion);
        }

    } catch (err) {
        const errorContext = { error: err };

        if (response && (response.status >= 300 || response.status < 200)) {
            errorContext.header = `Couldn't load patcher export bundle`;
            errorContext.description = `Trying to load "${patchExportURL}". Modify app.js if needed.`;
        }
        if (typeof guardrails === "function") guardrails(errorContext);
        else throw err;
        return;
    }
    
    let dependencies = [];
    try {
        const dependenciesResponse = await fetch("export/dependencies.json");
        dependencies = await dependenciesResponse.json();
        dependencies = dependencies.map(d => d.file ? Object.assign({}, d, { file: "export/" + d.file }) : d);
    } catch (e) {}

    let device;
    try {
        device = await RNBO.createDevice({ context, patcher });

        window.rnboDevice = device;
        window.rnboContext = context;

    } catch (err) {
        if (typeof guardrails === "function") guardrails({ error: err });
        else throw err;
        return;
    }

    if (dependencies.length)
        await device.loadDataBufferDependencies(dependencies);

    device.node.connect(outputNode);

    // ★★★ タイトル書き換えを無効化（重要） ★★★
    // document.getElementById("patcher-title").innerText = (...);

    makeSliders(device);
    makeInportForm(device);
    loadPresets(device, patcher);
    makeMIDIKeyboard(device);

    document.body.onclick = () => context.resume();

    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const micSource = context.createMediaStreamSource(stream);
        micSource.connect(device.node);
        console.log("Microphone connected to RNBO device.");
    } catch (err) {
        console.error("Microphone access failed:", err);
    }

    if (typeof guardrails === "function") guardrails();
}

function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        if (/^\d+\.\d+\.\d+-dev$/.test(version)) {
            throw new Error("Patcher exported with a Debug Version!");
        }
        const el = document.createElement("script");
        el.src = "https://c74-public.nyc3.digitaloceanspaces.com/rnbo/" + encodeURIComponent(version) + "/rnbo.min.js";
        el.onload = resolve;
        el.onerror = () => reject(new Error("Failed to load rnbo.js v" + version));
        document.body.append(el);
    });
}


/* ─────────────────────────────
   ★ ここが UI カスタムの本体 ★
   スライダー反転 ＋ ステップ丸め処理
────────────────────────────── */
function makeSliders(device) {

    const parameterRanges = {
        'volume': { min: 0, max: 1, initial: 0 },
        'sensitivity': { min: 20, max: 80, initial: 40 },
        'responsiveness': { min: 0, max: 10, initial: 5 },
        'dynamics': { min: 0, max: 4, initial: 2 },
        'release': { min: 0, max: 10, initial: 5 }
    };

    const stepTable = {
        volume: 0.01,
        sensitivity: 5,
        responsiveness: 1,
        dynamics: 1,
        release: 1
    };

    const sliderMappings = [
        { sliderId: 'volume-slider', paramName: 'volume' },
        { sliderId: 'sensitivity-slider', paramName: 'sensitivity' },
        { sliderId: 'dynamics-slider', paramName: 'dynamics' },
        { sliderId: 'responsiveness-slider', paramName: 'responsiveness' },
        { sliderId: 'release-slider', paramName: 'release' }
    ];

    sliderMappings.forEach(({ sliderId, paramName }) => {
        const slider = document.getElementById(sliderId);
        if (!slider) return;

        let param = device.parameters.find(p => p.name === paramName)
                  || device.parameters.find(p => p.id === paramName);

        if (!param) return;

        const range = parameterRanges[paramName];
        const step = stepTable[paramName] || 1;

        const isReversed = (paramName === "sensitivity" || paramName === "responsiveness");

        // RNBO → Web UI
        const paramToSlider = (value) => {
            const base = ((value - range.min) / (range.max - range.min)) * 100;
            return isReversed ? (100 - base) : base;
        };

        // Web UI → RNBO
        const sliderToParam = (value) => {

            // 逆転
            const raw = isReversed
                ? range.min + ((100 - value) / 100) * (range.max - range.min)
                : range.min + (value / 100) * (range.max - range.min);

            // 丸め処理（ステップ化）
            return Math.round(raw / step) * step;
        };

        // 初期値
        slider.value = paramToSlider(param.value);

        // スライダー操作 → RNBO
        slider.addEventListener('input', (e) => {
            const newVal = sliderToParam(parseFloat(e.target.value));
            param.value = newVal;
        });

        // RNBO 値 → スライダー反映
        setInterval(() => {
            const expected = paramToSlider(param.value);
            if (Math.abs(slider.value - expected) > 1) {
                slider.value = expected;
            }
        }, 100);
    });
}


/* 以下はデフォルト（変更なし） */
function makeInportForm(device) {
    const idiv = document.getElementById("rnbo-inports");
    const inportSelect = document.getElementById("inport-select");
    const inportText = document.getElementById("inport-text");
    const inportForm = document.getElementById("inport-form");

    if (!idiv) return;

    // RNBO inports を取得
    const inports = device.messages
        .filter(m => m.type === RNBO.MessagePortType.Inport);

    // ---- 安全な removeChild ----
    const safeRemove = (parent, selector) => {
        if (!parent) return;
        const node = parent.querySelector(selector);
        if (node && node.parentNode === parent) {
            parent.removeChild(node);
        }
    };
    // ----------------------------

    // inport が 0 の場合 → UIを消す
    if (inports.length === 0) {
        safeRemove(idiv, "#inport-form");
        safeRemove(idiv, "#no-inports-label");
        return;
    }

    // ラベルを削除
    safeRemove(idiv, "#no-inports-label");

    // セレクト生成
    inports.forEach(inport => {
        const option = document.createElement("option");
        option.textContent = inport.tag;
        inportSelect.appendChild(option);
    });

    let inportTag = inportSelect.value;

    inportSelect.onchange = () => {
        inportTag = inportSelect.value;
    };

    // 送信処理
    inportForm.onsubmit = (ev) => {
        ev.preventDefault();
        const values = inportText.value.split(/\s+/).map(num => parseFloat(num));
        const msg = new RNBO.MessageEvent(RNBO.TimeNow, inportTag, values);
        device.scheduleEvent(msg);
    };
}


function loadPresets(device, patcher) {
    let presets = patcher.presets || [];
    if (presets.length < 1) {
        document.getElementById("rnbo-presets").removeChild(document.getElementById("preset-select"));
        return;
    }

    document.getElementById("rnbo-presets").removeChild(document.getElementById("no-presets-label"));
    let presetSelect = document.getElementById("preset-select");
    presets.forEach((preset, index) => {
        const option = document.createElement("option");
        option.innerText = preset.name;
        option.value = index;
        presetSelect.appendChild(option);
    });
    presetSelect.onchange = () => device.setPreset(presets[presetSelect.value].preset);
}

function makeMIDIKeyboard(device) {
    let mdiv = document.getElementById("rnbo-clickable-keyboard");
    if (device.numMIDIInputPorts === 0) return;

    mdiv.removeChild(document.getElementById("no-midi-label"));

    const midiNotes = [49, 52, 56, 63];
    midiNotes.forEach(note => {
        const key = document.createElement("div");
        const label = document.createElement("p");
        label.textContent = note;
        key.appendChild(label);
        key.addEventListener("pointerdown", () => {
            let midiChannel = 0;

            let noteOnMessage = [144 + midiChannel, note, 100];
            let noteOffMessage = [128 + midiChannel, note, 0];

            let midiPort = 0;
            let noteDurationMs = 250;
        
            let noteOnEvent = new RNBO.MIDIEvent(device.context.currentTime * 1000, midiPort, noteOnMessage);
            let noteOffEvent = new RNBO.MIDIEvent(device.context.currentTime * 1000 + noteDurationMs, midiPort, noteOffMessage);
        
            device.scheduleEvent(noteOnEvent);
            device.scheduleEvent(noteOffEvent);

            key.classList.add("clicked");
        });

        key.addEventListener("pointerup", () => key.classList.remove("clicked"));
        mdiv.appendChild(key);
    });
}

setup();
