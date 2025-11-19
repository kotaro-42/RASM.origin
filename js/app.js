// RNBO パラメータとUIスライダーを接続
(async () => {
    // RNBOデバイスとコンテキストの準備を待つ
    while (!window.rnboDevice || !window.rnboContext) {
        await new Promise(resolve => setTimeout(resolve, 50));
    }
    
    // パラメータが完全にロードされるまでさらに待機
    await new Promise(resolve => setTimeout(resolve, 200));
    
    const context = window.rnboContext;
    const device = window.rnboDevice;
    
    // マイク入力接続
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        const micSource = context.createMediaStreamSource(stream);
        micSource.connect(device.node);
        console.log("✓ Microphone successfully connected to RNBO input.");
    } catch (e) {
        console.error("✗ Microphone access error:", e);
    }
    
    // 利用可能なパラメータを確認
    console.log("=== Available RNBO parameters ===");
    device.parameters.forEach((p, i) => {
        console.log(`[${i}] ${p.name}: ${p.value} (range: ${p.min} - ${p.max})`);
    });
    console.log("================================");
    
    // 各パラメータの正確な範囲と初期値
    const parameterRanges = {
        'volume': { min: 0, max: 1, initial: 0 },
        'sensitivity': { min: 20, max: 80, initial: 40 },
        'responsiveness': { min: 0, max: 10, initial: 5 },
        'dynamics': { min: 0, max: 4, initial: 2 },
        'release': { min: 0, max: 10, initial: 5 }
    };
    
    // スライダーとRNBOパラメータの接続
    const sliderMappings = [
        { sliderId: 'volume-slider', paramName: 'volume' },
        { sliderId: 'sensitivity-slider', paramName: 'sensitivity' },
        { sliderId: 'dynamics-slider', paramName: 'dynamics' },
        { sliderId: 'responsiveness-slider', paramName: 'responsiveness' },
        { sliderId: 'release-slider', paramName: 'release' }
    ];
    
    console.log("=== Connecting sliders ===");
    
    sliderMappings.forEach(({ sliderId, paramName }) => {
        const slider = document.getElementById(sliderId);
        if (!slider) {
            console.error(`✗ Slider element "${sliderId}" not found in DOM`);
            return;
        }
        
        // パラメータを名前とインデックスの両方で検索
        let param = device.parameters.find(p => p.name === paramName);
        if (!param) {
            param = device.parameters.find(p => p.id === paramName);
        }
        
        if (param) {
            const range = parameterRanges[paramName];
            console.log(`✓ Connecting "${sliderId}" → parameter "${param.name}"`);
            console.log(`  Range: ${range.min} to ${range.max}, Current: ${param.value}`);
            
            // パラメータをスライダーの0-100範囲に正規化
            const paramToSlider = (value) => {
                const normalized = ((value - range.min) / (range.max - range.min)) * 100;
                return Math.max(0, Math.min(100, normalized));
            };
            
            // スライダーの0-100値をパラメータの範囲に変換
            const sliderToParam = (value) => {
                const paramValue = range.min + (value / 100) * (range.max - range.min);
                return Math.max(range.min, Math.min(range.max, paramValue));
            };
            
            // 初期値設定
            const initialSliderValue = paramToSlider(param.value);
            slider.value = initialSliderValue;
            
            // スライダー変更時にパラメータ更新
            slider.addEventListener('input', (e) => {
                const sliderValue = parseFloat(e.target.value);
                const paramValue = sliderToParam(sliderValue);
                param.value = paramValue;
            });
            
            // パラメータ変更の監視
            setInterval(() => {
                const currentParamValue = param.value;
                const expectedSliderValue = paramToSlider(currentParamValue);
                
                if (Math.abs(slider.value - expectedSliderValue) > 1) {
                    slider.value = expectedSliderValue;
                }
            }, 100);
        } else {
            console.error(`✗ Parameter "${paramName}" NOT FOUND in device.parameters!`);
        }
    });

    console.log("=========================");
    console.log("Setup complete! Try moving the sliders.");

    /* ============================================================
       ★ 追加部分：Volume Gauge 同期コード
       ============================================================ */

    const gaugeFill = document.getElementById("volume-gauge-fill");

    const updateGaugeFromParam = () => {
        const volumeParam = device.parameters.find(p => p.name === "volume");
        if (!volumeParam) return;

        const normalized = volumeParam.value; // volumeは0〜1
        gaugeFill.style.height = (normalized * 100) + "%";
    };

    // スライダー操作時
    const volSlider = document.getElementById("volume-slider");
    volSlider.addEventListener("input", updateGaugeFromParam);

    // パラメータ外部変更時
    setInterval(updateGaugeFromParam, 100);

})();
