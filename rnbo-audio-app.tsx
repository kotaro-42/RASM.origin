import React, { useState, useEffect, useRef } from 'react';
import { Volume2, Play, Square, Settings } from 'lucide-react';

const RNBOAudioApp = () => {
  const [isRunning, setIsRunning] = useState(false);
  const [audioLevel, setAudioLevel] = useState(0);
  const [params, setParams] = useState({
    volume: 0,
    sensitivity: 40,
    responsiveness: 5,
    dynamics: 2,
    release: 5
  });
  
  const audioContextRef = useRef(null);
  const analyserRef = useRef(null);
  const animationRef = useRef(null);
  const canvasRef = useRef(null);

  // パラメータ設定
  const paramConfig = {
    volume: { min: 0, max: 1, step: 0.01, unit: '' },
    sensitivity: { min: 20, max: 80, step: 1, unit: '' },
    responsiveness: { min: 0, max: 10, step: 0.1, unit: '' },
    dynamics: { min: 0, max: 4, step: 0.1, unit: '' },
    release: { min: 0, max: 10, step: 0.1, unit: '' }
  };

  const handleParamChange = (param, value) => {
    setParams(prev => ({ ...prev, [param]: parseFloat(value) }));
  };

  const toggleAudio = async () => {
    if (!isRunning) {
      try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        audioContextRef.current = new (window.AudioContext || window.webkitAudioContext)();
        const source = audioContextRef.current.createMediaStreamSource(stream);
        
        analyserRef.current = audioContextRef.current.createAnalyser();
        analyserRef.current.fftSize = 2048;
        source.connect(analyserRef.current);
        
        setIsRunning(true);
        visualize();
      } catch (err) {
        console.error('Audio access error:', err);
        alert('マイクへのアクセスが必要です');
      }
    } else {
      if (audioContextRef.current) {
        audioContextRef.current.close();
        audioContextRef.current = null;
      }
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
      setIsRunning(false);
      setAudioLevel(0);
    }
  };

  const visualize = () => {
    if (!analyserRef.current) return;
    
    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    const bufferLength = analyserRef.current.frequencyBinCount;
    const dataArray = new Uint8Array(bufferLength);
    
    const draw = () => {
      animationRef.current = requestAnimationFrame(draw);
      analyserRef.current.getByteTimeDomainData(dataArray);
      
      // オーディオレベル計算
      let sum = 0;
      for (let i = 0; i < bufferLength; i++) {
        const normalized = (dataArray[i] - 128) / 128;
        sum += normalized * normalized;
      }
      const rms = Math.sqrt(sum / bufferLength);
      setAudioLevel(Math.min(rms * 5, 1));
      
      // 波形描画
      ctx.fillStyle = 'rgb(15, 23, 42)';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      
      ctx.lineWidth = 2;
      ctx.strokeStyle = 'rgb(59, 130, 246)';
      ctx.beginPath();
      
      const sliceWidth = canvas.width / bufferLength;
      let x = 0;
      
      for (let i = 0; i < bufferLength; i++) {
        const v = dataArray[i] / 128.0;
        const y = v * canvas.height / 2;
        
        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
        
        x += sliceWidth;
      }
      
      ctx.lineTo(canvas.width, canvas.height / 2);
      ctx.stroke();
    };
    
    draw();
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
  }, []);

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-slate-900 text-white p-6">
      <div className="max-w-6xl mx-auto">
        {/* ヘッダー */}
        <div className="mb-8">
          <div className="flex items-center gap-3 mb-2">
            <Settings className="w-8 h-8 text-blue-400" />
            <h1 className="text-4xl font-bold bg-clip-text text-transparent bg-gradient-to-r from-blue-400 to-purple-400">
              RNBO Audio Processor
            </h1>
          </div>
          <p className="text-slate-400">リアルタイムオーディオ処理とパラメータコントロール</p>
        </div>

        {/* メインコンテンツ */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* 左側: オーディオビジュアライザー */}
          <div className="space-y-6">
            {/* 波形表示 */}
            <div className="bg-slate-800/50 backdrop-blur-sm rounded-2xl p-6 border border-slate-700/50 shadow-xl">
              <h2 className="text-xl font-semibold mb-4 flex items-center gap-2">
                <Volume2 className="w-5 h-5 text-blue-400" />
                オーディオ入力波形
              </h2>
              <canvas 
                ref={canvasRef}
                className="w-full h-48 rounded-lg bg-slate-950"
              />
              
              {/* オーディオレベルメーター */}
              <div className="mt-4">
                <div className="flex justify-between text-sm text-slate-400 mb-2">
                  <span>入力レベル</span>
                  <span>{(audioLevel * 100).toFixed(0)}%</span>
                </div>
                <div className="h-3 bg-slate-950 rounded-full overflow-hidden">
                  <div 
                    className="h-full bg-gradient-to-r from-green-500 via-yellow-500 to-red-500 transition-all duration-100"
                    style={{ width: `${audioLevel * 100}%` }}
                  />
                </div>
              </div>
            </div>

            {/* コントロールボタン */}
            <div className="bg-slate-800/50 backdrop-blur-sm rounded-2xl p-6 border border-slate-700/50 shadow-xl">
              <button
                onClick={toggleAudio}
                className={`w-full py-4 rounded-xl font-semibold text-lg transition-all duration-300 flex items-center justify-center gap-3 ${
                  isRunning
                    ? 'bg-red-500 hover:bg-red-600 shadow-lg shadow-red-500/50'
                    : 'bg-blue-500 hover:bg-blue-600 shadow-lg shadow-blue-500/50'
                }`}
              >
                {isRunning ? (
                  <>
                    <Square className="w-6 h-6" />
                    停止
                  </>
                ) : (
                  <>
                    <Play className="w-6 h-6" />
                    開始
                  </>
                )}
              </button>
              <p className="text-center text-sm text-slate-400 mt-3">
                {isRunning ? 'オーディオ処理実行中...' : 'マイク入力を使用します'}
              </p>
            </div>
          </div>

          {/* 右側: パラメータコントロール */}
          <div className="bg-slate-800/50 backdrop-blur-sm rounded-2xl p-6 border border-slate-700/50 shadow-xl">
            <h2 className="text-xl font-semibold mb-6">パラメータ設定</h2>
            
            <div className="space-y-6">
              {Object.entries(params).map(([key, value]) => {
                const config = paramConfig[key];
                const displayName = {
                  volume: 'ボリューム',
                  sensitivity: '感度',
                  responsiveness: '応答性',
                  dynamics: 'ダイナミクス',
                  release: 'リリース'
                }[key];
                
                return (
                  <div key={key} className="space-y-2">
                    <div className="flex justify-between items-center">
                      <label className="text-sm font-medium text-slate-300">
                        {displayName}
                      </label>
                      <span className="text-sm font-mono bg-slate-950 px-3 py-1 rounded-lg text-blue-400">
                        {value.toFixed(key === 'volume' ? 2 : 1)}{config.unit}
                      </span>
                    </div>
                    <input
                      type="range"
                      min={config.min}
                      max={config.max}
                      step={config.step}
                      value={value}
                      onChange={(e) => handleParamChange(key, e.target.value)}
                      className="w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-blue-500"
                      style={{
                        background: `linear-gradient(to right, rgb(59, 130, 246) 0%, rgb(59, 130, 246) ${((value - config.min) / (config.max - config.min)) * 100}%, rgb(51, 65, 85) ${((value - config.min) / (config.max - config.min)) * 100}%, rgb(51, 65, 85) 100%)`
                      }}
                    />
                    <div className="flex justify-between text-xs text-slate-500">
                      <span>{config.min}{config.unit}</span>
                      <span>{config.max}{config.unit}</span>
                    </div>
                  </div>
                );
              })}
            </div>

            {/* パラメータ説明 */}
            <div className="mt-6 pt-6 border-t border-slate-700">
              <h3 className="text-sm font-semibold text-slate-400 mb-3">パラメータ説明</h3>
              <div className="space-y-2 text-xs text-slate-500">
                <p><span className="text-slate-400 font-medium">ボリューム:</span> 出力音量を調整</p>
                <p><span className="text-slate-400 font-medium">感度:</span> 入力信号の検出感度 (20-80)</p>
                <p><span className="text-slate-400 font-medium">応答性:</span> パラメータの反応速度</p>
                <p><span className="text-slate-400 font-medium">ダイナミクス:</span> ダイナミックレンジの制御</p>
                <p><span className="text-slate-400 font-medium">リリース:</span> エンベロープのリリースタイム</p>
              </div>
            </div>
          </div>
        </div>

        {/* フッター情報 */}
        <div className="mt-8 text-center text-sm text-slate-500">
          <p>RNBO Audio Processor - Cycling '74 RNBO Export</p>
          <p className="mt-1">5つのパラメータでリアルタイムオーディオ処理を制御</p>
        </div>
      </div>
    </div>
  );
};

export default RNBOAudioApp;