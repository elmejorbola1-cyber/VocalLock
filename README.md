# VocalLock – Auto-tune VST3 (JUCE)

Pitch correction en tiempo real: detección de tono YIN + pitch shifter de dominio temporal,
más una cadena de efectos pensada para el sonido "Travis Scott".

## Compilar
Necesitas: CMake ≥ 3.22, git y un compilador C++17 (Visual Studio 2022 / Xcode / gcc+ALSA dev libs).

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

(La primera vez descarga JUCE 8.0.4 automáticamente.)
El VST3 queda en `build/VocalLock_artefacts/Release/VST3/VocalLock.vst3`.
- Windows: copiar a `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`
- Linux: `~/.vst3/`

También se genera un ejecutable Standalone para probar sin DAW.

## Parámetros
| Parámetro | Qué hace |
|---|---|
| Key / Scale | Notas a las que se "pega" la voz. **Pon la tonalidad de tu beat.** |
| Retune Speed | 0 ms = efecto duro/robótico; 20–40 ms = natural |
| Flex-Tune | Si la nota ya está a menos de X cents, no se toca |
| Humanize | Alarga el tiempo de corrección en notas sostenidas |
| Low Layer | Capa una octava abajo (típica de producciones tipo Utopia/Astroworld) |
| Doubler | Doble voz modulada en estéreo |
| Drive | Saturación tanh |
| Delay Time/FB/Mix | Ping-pong con filtro en el feedback |
| Reverb Size/Mix | Reverb de sala grande |
| Output | Ganancia de salida |

## Preset "Travis Scott"
Retune 0 ms, Flex 0, Humanize 0, Low Layer 30 %, Doubler 40 %, Drive 30 %,
Delay 330 ms / 38 % FB / 24 % mix, Reverb 75 % / 25 % mix.
Los presets no cambian Key/Scale.

Cadena sugerida en el DAW: HPF ~100 Hz → compresor suave → **VocalLock** → EQ (+aire a 10–12 kHz, recorte de 250–400 Hz si suena embarrado) → de-esser → limitador.

## Notas
- Procesa la entrada en mono (suma L+R) y saca estéreo.
- Latencia ≈ 20–30 ms, no se reporta al DAW: úsalo en pistas de voz ya grabadas o con monitoreo directo.
- Funciona mejor con voz monofónica y limpia (sin reverb previa).
