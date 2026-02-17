console.log('JavaScript file loaded successfully!');

function testRequest() {
    fetch('/api/test')
        .then(response => response.json())
        .then(data => console.log('API Response:', data))
        .catch(error => console.error('Error:', error));
}
