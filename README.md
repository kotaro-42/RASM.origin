# RNBO Audio Processor

RNBOオーディオプロセッサとReact UI

## ファイル

- `rnbo_source.cpp` - RNBO生成のC++オーディオ処理コード
- `rnbo-audio-app.tsx` - パラメータコントロールUI（React）

## パラメータ

- Volume: 0.0 - 1.0
- Sensitivity: 20 - 80
- Responsiveness: 0 - 10
- Dynamics: 0 - 4
- Release: 0 - 10

## 使用方法

UIはClaude.aiのArtifactやReactプロジェクトで直接使用できます。
C++コードは別途ビルド環境で利用します。
