let currentSpeed = 80;

// Referencias a elementos
const speedSlider = document.getElementById('speed-slider');
const speedValue = document.getElementById('speed-value');
const statusText = document.getElementById('status-text');

// Actualizar valor de velocidad
speedSlider.addEventListener('input', (e) => {
    currentSpeed = parseInt(e.target.value);
    speedValue.textContent = currentSpeed;
});

// Función para enviar comandos al servidor
async function sendCommand(action) {
    try {
        const response = await fetch('/api/control', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                action: action,
                speed: currentSpeed
            })
        });
        
        const data = await response.json();
        
        if (response.ok) {
            updateStatus(action);
        } else {
            console.error('Error:', data.error);
            statusText.textContent = 'Error: ' + data.error;
        }
    } catch (error) {
        console.error('Error:', error);
        statusText.textContent = 'Error de conexión';
    }
}

function updateStatus(action) {
    const statusMap = {
        'forward': 'Avanzando',
        'backward': 'Retrocediendo',
        'left': 'Girando izquierda',
        'right': 'Girando derecha',
        'stop': 'Detenido'
    };
    statusText.textContent = statusMap[action] || 'Desconocido';
    
    // Actualiza indicadores visuales de dirección
    document.querySelectorAll('.direction-arrow').forEach(arrow => {
        arrow.classList.remove('active');
    });
    
    const arrowMap = {
        'forward': 'arrow-forward',
        'backward': 'arrow-backward',
        'left': 'arrow-left',
        'right': 'arrow-right',
        'stop': 'arrow-center'
    };
    
    if (arrowMap[action]) {
        document.getElementById(arrowMap[action]).classList.add('active');
    }
    
    // Actualiza velocímetro visual
    const speedDisplay = document.getElementById('speed-display');
    const speedBar = document.getElementById('speed-bar');
    
    if (action === 'stop') {
        speedDisplay.textContent = '0';
        speedBar.style.width = '0%';
    } else {
        speedDisplay.textContent = currentSpeed;
        speedBar.style.width = currentSpeed + '%';
    }
}

// Event listeners para botones
document.getElementById('btn-forward').addEventListener('click', () => sendCommand('forward'));
document.getElementById('btn-backward').addEventListener('click', () => sendCommand('backward'));
document.getElementById('btn-left').addEventListener('click', () => sendCommand('left'));
document.getElementById('btn-right').addEventListener('click', () => sendCommand('right'));
document.getElementById('btn-stop').addEventListener('click', () => sendCommand('stop'));

// Control con teclado
document.addEventListener('keydown', (e) => {
    switch(e.key) {
        case 'ArrowUp':
        case 'w':
            sendCommand('forward');
            break;
        case 'ArrowDown':
        case 's':
            sendCommand('backward');
            break;
        case 'ArrowLeft':
        case 'a':
            sendCommand('left');
            break;
        case 'ArrowRight':
        case 'd':
            sendCommand('right');
            break;
        case ' ':
            sendCommand('stop');
            e.preventDefault();
            break;
    }
});

// Canvas para gráfica PWM
const canvas = document.getElementById('pwm-wave');
const ctx = canvas.getContext('2d');
canvas.width = canvas.offsetWidth;
canvas.height = 100;

function drawPWM(dutyCycle) {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Configuración
    const periods = 4; // Mostrar 4 períodos
    const periodWidth = canvas.width / periods;
    const highTime = periodWidth * (dutyCycle / 100);
    const lowTime = periodWidth - highTime;
    
    // Dibuja onda PWM
    ctx.strokeStyle = '#667eea';
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    let x = 0;
    for (let i = 0; i < periods; i++) {
        // High
        ctx.moveTo(x, 80);
        ctx.lineTo(x, 20);
        ctx.lineTo(x + highTime, 20);
        
        // Low
        ctx.lineTo(x + highTime, 80);
        ctx.lineTo(x + periodWidth, 80);
        
        x += periodWidth;
    }
    
    ctx.stroke();
    
    // Actualiza duty cycle display
    document.getElementById('duty-cycle').textContent = dutyCycle;
}

// Llama a drawPWM cuando cambia la velocidad
speedSlider.addEventListener('input', (e) => {
    currentSpeed = parseInt(e.target.value);
    speedValue.textContent = currentSpeed;
    drawPWM(currentSpeed);
});

// Actualiza la función sendCommand para redibujar PWM
const originalUpdateStatus = updateStatus;
updateStatus = function(action) {
    originalUpdateStatus(action);
    
    if (action === 'stop') {
        drawPWM(0);
    } else {
        drawPWM(currentSpeed);
    }
};

// Dibuja PWM inicial
drawPWM(currentSpeed);

// Captura de imagen
document.getElementById('btn-capture').addEventListener('click', async () => {
    const captureBtn = document.getElementById('btn-capture');
    const capturedImageDiv = document.getElementById('captured-image');
    
    captureBtn.disabled = true;
    captureBtn.textContent = '📷 Capturando...';
    
    try {
        const response = await fetch('/api/capture', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });
        
        const data = await response.json();
        
        if (response.ok) {
            capturedImageDiv.innerHTML = `
                <p>✓ Imagen capturada: ${data.filename}</p>
                <img src="data:image/jpeg;base64,${data.image}" alt="Captured">
            `;
        } else {
            capturedImageDiv.innerHTML = `<p style="color: red;">Error: ${data.error}</p>`;
        }
    } catch (error) {
        capturedImageDiv.innerHTML = `<p style="color: red;">Error de conexión</p>`;
    } finally {
        captureBtn.disabled = false;
        captureBtn.textContent = '📷 Capturar Imagen';
    }
});

// Monitoreo de ultrasonido
setInterval(async () => {
    try {
        const response = await fetch('/api/ultrasonic');
        const data = await response.json();
        
        document.getElementById('distance-value').textContent = 
            data.distance > 0 ? data.distance : '--';
        
        const alert = document.getElementById('obstacle-alert');
        if (data.obstacle_detected) {
            alert.style.display = 'block';
        } else {
            alert.style.display = 'none';
        }
    } catch (error) {
        console.error('Ultrasonic error:', error);
    }
}, 200); // Actualiza cada 200ms

// Control de modo de luces
const lightsMode = document.querySelectorAll('input[name="lights-mode"]');
const manualLights = document.getElementById('manual-lights');

lightsMode.forEach(radio => {
    radio.addEventListener('change', async (e) => {
        const isAuto = e.target.value === 'auto';
        manualLights.style.display = isAuto ? 'none' : 'block';
        
        await fetch('/api/lights/mode', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({auto: isAuto})
        });
    });
});

// Control manual de luces
document.querySelectorAll('.light-btn').forEach(btn => {
    btn.addEventListener('click', async () => {
        const action = btn.dataset.light;
        
        await fetch('/api/lights/control', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({action: action})
        });
    });
});