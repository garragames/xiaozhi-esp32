# Notas de Depuración e Investigación (Kotty ESP32-C3)

Este documento recopila las observaciones, hipótesis y soluciones aplicadas durante la sesión de depuración del 20 de febrero de 2026, centrada en la estabilidad del Wake Word y el ciclo de conversación.

## 1. Problema: Wake Word "Sordo" en Idle
**Síntoma:** El dispositivo detectaba el Wake Word ("Alexa") correctamente tras el arranque, pero después de la primera interacción completa, al volver al estado de espera (`Idle` / Icono Reloj), dejaba de responder.
**Causa Raíz Identificada:** Condición de carrera en el ESP32-C3 (recursos limitados). Al transicionar a `Idle`, el apagado del `VoiceProcessor` y el encendido del `WakeWordDetector` ocurrían casi simultáneamente. El detector intentaba inicializarse sobre hardware/buffers que aún no habían sido liberados por la tarea de audio.
**Solución Aplicada:**
*   Se introdujo un `vTaskDelay(100)` en `Application::HandleStateChangedEvent` (caso `kDeviceStateIdle`) antes de habilitar el Wake Word.
*   Se forzó la desactivación explícita de ambos procesos en el estado `Connecting` para liberar RAM para SSL.
*   Se ajustó el Sample Rate a **16000 Hz** en `config.h` (antes 24000 Hz) para coincidir con el modelo nativo y evitar el uso de resamplers costosos.

## 2. Problema: Caída prematura a Idle (Intermitente)
**Síntoma:** Ocasionalmente, tras responder una pregunta (`Speaking`), el dispositivo vuelve inmediatamente a `Idle` en lugar de quedarse escuchando (`Listening`) para continuar la conversación, rompiendo el flujo de "chat continuo".
**Análisis:**
*   La transición a `Idle` solo ocurre si se llama explícitamente a `SetDeviceState(kDeviceStateIdle)`.
*   Los disparadores posibles son:
    1.  **Servidor (Protocolo):** El servidor cierra el canal de audio (`OnAudioChannelClosed`). Esto puede deberse a timeouts, errores de protocolo o decisión lógica del servidor.
    2.  **VAD (Voice Activity Detection):** Si el servidor o el dispositivo detectan silencio inmediatamente tras el TTS, pueden interpretar fin de sesión.
    3.  **Red:** Desconexión momentánea del WebSocket/MQTT.
*   **Hipótesis Actual:** Es probable que sea un comportamiento dependiente del servidor (`xiaozhi-esp32-server`) o latencia de red, más que un bug en la máquina de estados del firmware. Los cambios recientes en `Idle` (el delay) ocurren *después* de que se toma la decisión de ir a `Idle`, por lo que no deberían ser la causa.

## 3. Modos de Escucha
*   **AutoStop:** (Por defecto si AEC desactivado). Escucha una frase -> Responde -> Duerme.
*   **Realtime:** (Por defecto si AEC activado). Escucha -> Responde -> Sigue escuchando.
*   **Nota:** El servidor puede enviar un comando `stop` en el evento TTS. Si el modo es `AutoStop`, el firmware va a `Idle`. Si es `Realtime` (o si el código fuerza la continuidad), intenta volver a `Listening`.

## 4. Plan de Pruebas y Siguientes Pasos
Se recomienda realizar pruebas intensivas para aislar el problema de la caída a Idle:
*   **Monitorear Logs:** Buscar mensajes como `Closing audio channel due to...` o `Network disconnected` justo cuando ocurre el fallo.
*   **Verificar Servidor:** Confirmar si el servidor está cerrando la conexión intencionalmente.
*   **Validar Estabilidad:** Confirmar que la solución del Wake Word (punto 1) es robusta a largo plazo.

Si el problema persiste y se confirma que no es el servidor:
*   Revisar la lógica de `Application::HandleStateChangedEvent` para asegurar que `audio_service_` no quede en estado inconsistente.
*   Considerar forzar `kListeningModeRealtime` ignorando la configuración de AEC si el eco no es problemático.
