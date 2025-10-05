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

// Actualizar texto de estado
function updateStatus(action) {
    const statusMap = {
        'forward': 'Avanzando',
        'backward': 'Retrocediendo',
        'left': 'Girando izquierda',
        'right': 'Girando derecha',
        'stop': 'Detenido'
    };
    statusText.textContent = statusMap[action] || 'Desconocido';
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