document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('upload-form');
    const statusDiv = document.getElementById('upload-status');
    const debugToggle = document.getElementById('debug-toggle');
    const debugPanel = document.getElementById('debug-panel');
    const debugRequest = document.getElementById('debug-request');
    const debugResponse = document.getElementById('debug-response');
    
    // Toggle debug panel visibility
    debugToggle.addEventListener('change', function() {
        debugPanel.classList.toggle('hidden', !this.checked);
    });
    
    form.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const fileInput = document.getElementById('file-upload');
        const file = fileInput.files[0];
        
        if (!file) {
            statusDiv.textContent = 'Please select a file to upload';
            return;
        }
        
        // Create FormData object to send the file
        const formData = new FormData();
        formData.append('file', file);
        
        statusDiv.textContent = 'Uploading...';
        
        // Clear previous debug info
        debugRequest.textContent = '';
        debugResponse.textContent = '';
        
        try {
            // Debug info for request
            const requestDebugInfo = `File: ${file.name}
Size: ${formatFileSize(file.size)}
Type: ${file.type}
Last Modified: ${new Date(file.lastModified).toLocaleString()}

FormData contains:
- Name: file
- Filename: ${file.name}
`;
            debugRequest.textContent = requestDebugInfo;
            
            // Use XMLHttpRequest instead of fetch for better debugging
            const xhr = new XMLHttpRequest();
            
            // Set up progress tracking
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = Math.round((e.loaded / e.total) * 100);
                    statusDiv.textContent = `Uploading: ${percentComplete}%`;
                }
            });
            
            // Promise wrapper for XMLHttpRequest
            const uploadPromise = new Promise((resolve, reject) => {
                xhr.onload = function() {
                    if (xhr.status >= 200 && xhr.status < 300) {
                        resolve({
                            ok: true,
                            status: xhr.status,
                            statusText: xhr.statusText,
                            text: () => Promise.resolve(xhr.responseText),
                            headers: parseHeaders(xhr.getAllResponseHeaders())
                        });
                    } else {
                        reject({
                            status: xhr.status,
                            statusText: xhr.statusText,
                            message: xhr.responseText
                        });
                    }
                };
                
                xhr.onerror = function() {
                    reject({
                        status: xhr.status,
                        statusText: 'Network Error',
                        message: 'Network request failed'
                    });
                };
                
                xhr.open('POST', '/upload', true);
                xhr.send(formData);
            });
            
            // Wait for upload to complete
            const response = await uploadPromise;
            const resultText = await response.text();
            
            // Debug info for response
            const responseDebugInfo = `Status: ${response.status} ${response.statusText}
Headers:
${formatHeaders(response.headers)}

Body:
${resultText}`;
            
            debugResponse.textContent = responseDebugInfo;
            
            // Update status
            if (response.ok) {
                statusDiv.textContent = 'Upload successful: ' + resultText;
                form.reset();
            } else {
                statusDiv.textContent = 'Upload failed: ' + response.status;
            }
        } catch (error) {
            // Debug info for error
            const errorDebugInfo = `Error: ${error.message || 'Unknown error'}
Status: ${error.status || 'N/A'}
Status Text: ${error.statusText || 'N/A'}`;
            
            debugResponse.textContent = errorDebugInfo;
            statusDiv.textContent = `Upload error: ${error.message || 'Unknown error'}`;
            console.error('Upload error:', error);
        }
    });
    
    // Helper function to format file size
    function formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';
        
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
    
    // Helper function to parse headers from XMLHttpRequest
    function parseHeaders(headerStr) {
        const headers = {};
        const headerPairs = headerStr.trim().split('\r\n');
        
        headerPairs.forEach(function(line) {
            const parts = line.split(': ');
            const header = parts.shift();
            const value = parts.join(': ');
            headers[header] = value;
        });
        
        return headers;
    }
    
    // Helper function to format headers for display
    function formatHeaders(headers) {
        return Object.entries(headers).map(([key, value]) => `${key}: ${value}`).join('\n');
    }
});