// Main application JavaScript - Tab navigation and HTTP testing

function showTab(tabId, clickEvent) {
    // Hide all tabs
    const tabs = document.querySelectorAll('.tab-content');
    tabs.forEach(tab => tab.classList.remove('active'));

    // Remove active class from all buttons
    const buttons = document.querySelectorAll('.nav-btn');
    buttons.forEach(btn => btn.classList.remove('active'));

    // Show selected tab
    document.getElementById(tabId).classList.add('active');

    // Add active class to clicked button
    if (clickEvent) {
        clickEvent.target.classList.add('active');
    }
}

async function testMethod(method, url) {
    try {
        const options = {
            method: method,
            headers: {}
        };

        if (method === 'POST') {
            options.body = JSON.stringify({ data: 'test', timestamp: new Date().toISOString() });
            options.headers['Content-Type'] = 'application/json';
        }

        const response = await fetch(url, options);
        const data = await response.text();

        displayResult({
            method,
            url,
            status: response.status,
            statusText: response.statusText,
            body: data,
            timestamp: new Date().toISOString()
        });
    } catch (error) {
        displayResult({
            method,
            url,
            error: error.message,
            timestamp: new Date().toISOString()
        });
    }
}

function testCustomMethod() {
    const method = document.getElementById('methodSelect').value;
    const url = document.getElementById('urlInput').value;
    testMethod(method, url);
}

function displayResult(result) {
    const resultsDiv = document.getElementById('testResults');
    
    // Remove "no results" message if it exists
    const noResults = resultsDiv.querySelector('.no-results');
    if (noResults) {
        noResults.remove();
    }

    // Create result card
    const resultCard = document.createElement('div');
    resultCard.className = 'result-card';

    let statusBadge = '';
    if (result.status) {
        const statusClass = result.status >= 200 && result.status < 300 ? 'status-success' : 'status-error';
        statusBadge = `<span class="status-badge ${statusClass}">${result.status} ${result.statusText}</span>`;
    }

    resultCard.innerHTML = `
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem;">
            <div>
                <strong style="color: #c084fc;">${result.method}</strong>
                <code style="margin-left: 0.5rem;">${result.url}</code>
            </div>
            ${statusBadge}
        </div>
        ${result.error ? 
            `<div style="background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.3); border-radius: 0.5rem; padding: 1rem;">
                <p style="color: #ef4444;">Error: ${result.error}</p>
            </div>` :
            `<div style="background: rgba(0, 0, 0, 0.4); border-radius: 0.5rem; padding: 1rem;">
                <p style="color: #c084fc; font-size: 0.875rem; margin-bottom: 0.5rem;">Response Body:</p>
                <pre style="margin: 0; max-height: 200px; overflow-y: auto;">${result.body ? result.body.substring(0, 500) : 'Empty response'}${result.body && result.body.length > 500 ? '...' : ''}</pre>
            </div>`
        }
        <p style="color: #a855f7; font-size: 0.75rem; margin-top: 1rem;">${new Date(result.timestamp).toLocaleString()}</p>
    `;

    // Add to top of results
    resultsDiv.insertBefore(resultCard, resultsDiv.firstChild);

    // Keep only last 5 results
    while (resultsDiv.children.length > 5) {
        resultsDiv.removeChild(resultsDiv.lastChild);
    }
}

async function handleFileUpload(event) {
    const file = event.target.files[0];
    if (!file) return;

    const statusDiv = document.getElementById('uploadStatus');
    statusDiv.innerHTML = `
        <div style="margin-top: 1rem;">
            <div style="width: 100%; background: rgba(100, 116, 139, 0.3); border-radius: 9999px; height: 8px; overflow: hidden;">
                <div style="width: 100%; height: 100%; background: #a855f7; animation: pulse 1s infinite;"></div>
            </div>
            <p style="color: #c084fc; margin-top: 0.5rem;">Uploading ${file.name}...</p>
        </div>
    `;

    const formData = new FormData();
    formData.append('file', file);

    try {
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            statusDiv.innerHTML = `
                <div style="margin-top: 1rem; display: flex; align-items: center; justify-content: center; color: #22c55e;">
                    <span style="font-size: 1.5rem; margin-right: 0.5rem;">✓</span>
                    <span>Upload successful! File: ${file.name}</span>
                </div>
            `;
            
            // Also add to test results
            displayResult({
                method: 'POST',
                url: '/upload',
                status: response.status,
                statusText: 'OK',
                body: `File uploaded: ${file.name} (${(file.size / 1024).toFixed(2)} KB)`,
                timestamp: new Date().toISOString()
            });
        } else {
            throw new Error(`Upload failed with status ${response.status}`);
        }
    } catch (error) {
        statusDiv.innerHTML = `
            <div style="margin-top: 1rem; display: flex; align-items: center; justify-content: center; color: #ef4444;">
                <span style="font-size: 1.5rem; margin-right: 0.5rem;">✗</span>
                <span>Upload failed: ${error.message}</span>
            </div>
        `;
    }

    // Clear status after 5 seconds
    setTimeout(() => {
        statusDiv.innerHTML = '';
    }, 5000);
}

// Allow Enter key to submit custom request
document.addEventListener('DOMContentLoaded', function() {
    const urlInput = document.getElementById('urlInput');
    if (urlInput) {
        urlInput.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                testCustomMethod();
            }
        });
    }
});

// Fetch performance metrics from backend
async function fetchPerformanceMetrics() {
    try {
        const startTime = performance.now();
        
        // Call backend metrics endpoint
        const response = await fetch('/api/metrics', {
            method: 'GET'
        });

        const endTime = performance.now();
        const responseTime = endTime - startTime;

        if (response.ok) {
            const data = await response.json();
            
            // Update UI with real data
            document.getElementById('metric-connections').textContent = data.connections || '10,000+';
            document.getElementById('metric-response').textContent = Math.round(responseTime) + 'ms';
            document.getElementById('metric-uptime').textContent = data.uptime || '99.9%';
            
            // Show success message
            displayResult({
                method: 'GET',
                url: '/api/metrics',
                status: response.status,
                statusText: 'OK',
                body: JSON.stringify(data, null, 2),
                timestamp: new Date().toISOString()
            });
        } else {
            throw new Error(`Metrics request failed with status ${response.status}`);
        }
    } catch (error) {
        displayResult({
            method: 'GET',
            url: '/api/metrics',
            error: error.message,
            timestamp: new Date().toISOString()
        });
    }
}

// Toggle expandable resource categories
function toggleResource(headerElement) {
    const category = headerElement.closest('.resource-category');
    category.classList.toggle('expanded');
}

