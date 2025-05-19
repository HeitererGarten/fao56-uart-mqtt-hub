document.addEventListener('DOMContentLoaded', function() {
    // Load current configuration
    fetch('/config')
        .then(response => response.json())
        .then(data => {
            document.getElementById('hub_id').value = data.hub_id || '';
            document.getElementById('wifi_ssid').value = data.wifi_ssid || '';
            document.getElementById('mqtt_server').value = data.mqtt_server || '';
            document.getElementById('mqtt_port').value = data.mqtt_port || 1883;
            document.getElementById('mqtt_username').value = data.mqtt_username || '';
            document.getElementById('mqtt_password').value = data.mqtt_password || '';
        })
        .catch(error => {
            console.error('Error loading config:', error);
            showStatus('Error loading configuration', false);
        });
    
    // Handle form submission
    document.getElementById('configForm').addEventListener('submit', function(e) {
        e.preventDefault();
        
        const config = {
            hub_id: document.getElementById('hub_id').value,
            wifi_ssid: document.getElementById('wifi_ssid').value,
            wifi_password: document.getElementById('wifi_password').value,
            mqtt_server: document.getElementById('mqtt_server').value,
            mqtt_port: parseInt(document.getElementById('mqtt_port').value),
            mqtt_username: document.getElementById('mqtt_username').value,
            mqtt_password: document.getElementById('mqtt_password').value
        };
        
        fetch('/save', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(config),
        })
        .then(response => response.text())
        .then(data => {
            showStatus(data, true);
            // Restart automatically after successful save
            setTimeout(() => {
                window.location.reload();
            }, 3000);
        })
        .catch(error => {
            console.error('Error saving config:', error);
            showStatus('Error saving configuration', false);
        });
    });
    
    // Handle restart button
    document.getElementById('restartBtn').addEventListener('click', function() {
        if (confirm('Are you sure you want to restart the hub?')) {
            fetch('/restart', {
                method: 'POST'
            })
            .then(response => response.text())
            .then(data => {
                showStatus(data, true);
            })
            .catch(error => {
                showStatus('Error restarting device', false);
            });
        }
    });
    
    function showStatus(message, success) {
        const statusDiv = document.getElementById('status');
        statusDiv.textContent = message;
        statusDiv.className = success ? 'success' : 'error';
    }
});