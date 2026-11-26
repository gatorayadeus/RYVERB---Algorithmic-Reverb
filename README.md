# RYVRB — reverb algorítmica VST3

Reverb estéreo en tiempo real con 33 presets de fábrica y una interfaz azul fuerte + dorado
basada en el escudo de referencia. Motor FDN de 8 líneas con decaimiento por bandas.

## Controles
**Principales:** Pre Delay (0–250 ms) · Decay (0.1–20 s) · Mix · Output · Bypass · comparación A/B.

| Pestaña | Controles |
|---|---|
| **Damp** | Low Ratio / Low Freq (cuánto más dura la reverb en graves) · High Ratio / High Freq (cuánto más corta en agudos) |
| **Space** | Size (tamaño del espacio) · Diffusion · Modulation · Width (ancho estéreo) |
| **Ducker** | Amount · Threshold · Release · Attack (baja la reverb mientras suena la señal y la deja respirar después) |
| **EQ** | Low Cut · High Cut · Low Gain · High Gain (sobre la señal de reverb) |

**Presets:** 9 categorías (Default, Halls, Rooms, Plates, Chambers, Vocals, Drums, Ambient, Stadium).
Flechas para recorrer, o clic en el nombre del preset para ver el menú completo.

## Cómo obtener el .vst3 (GitHub Actions)
1. Crea un repositorio nuevo en GitHub y sube este proyecto (ver instrucciones con Git abajo).
2. Pestaña **Actions** → se ejecuta *Build RYVRB* (~6 min).
3. Descarga el artefacto **RYVRB-VST3-Windows** (o macOS / Linux).
4. Copia `RYVRB.vst3` a `C:\Program Files\Common Files\VST3\` y reescanea plugins en tu DAW.

```
git init
git add .
git commit -m "RYVRB v1.0"
git branch -M main
git remote add origin https://github.com/TU_USUARIO/TU_REPOSITORIO.git
git push -u origin main
```

## Compilar en tu equipo
- **Windows:** Visual Studio 2022 + CMake, luego `build_windows.bat` como administrador.
- **macOS:** `xcode-select --install`, `brew install cmake`, luego `./build_mac.sh`.

## Notas técnicas
- Matriz de realimentación Hadamard 8×8, difusión de entrada con 4 allpass, delays modulados.
- RT60 medido vs. knob Decay: error 1–4 % a 44.1/48/96/192 kHz. Estable con todos los controles al extremo.
- Latencia 0 muestras (el pre-delay es independiente). Cola máxima ~20 s.
- Probado offline (`tests/dsp_test.cpp`): 8 grupos de pruebas, incluidos los 33 presets. ~1.2 % de un núcleo.

## Licencias
JUCE 8 es AGPLv3 o licencia comercial: libre para uso personal; para distribuir públicamente hay que
publicar el código o adquirir licencia. El escudo es marca del club: úsalo solo de forma personal/no comercial.
