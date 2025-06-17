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
        
        // Display file size info for debugging
        const fileSizeMB = file.size / (1024 * 1024);
        const fileSizeGB = fileSizeMB / 1024;
        
        let fileSizeStr = '';
        if (fileSizeGB >= 1) {
            fileSizeStr = fileSizeGB.toFixed(2) + ' GB';
        } else {
            fileSizeStr = fileSizeMB.toFixed(2) + ' MB';
        }
        
        // Clear previous debug info
        debugRequest.textContent = '';
        debugResponse.textContent = '';
        
        // Debug info for request
        debugRequest.textContent = `File: ${file.name}
Size: ${fileSizeStr}
Type: ${file.type}
Last Modified: ${new Date(file.lastModified).toLocaleString()}

FormData contains:
- Name: file
- Filename: ${file.name}
`;
        
        // Create FormData object to send the file
        const formData = new FormData();
        formData.append('file', file);
        
        statusDiv.textContent = 'Uploading...';
        
        try {
            // Use XMLHttpRequest for better debugging and progress tracking
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
                    resolve({
                        status: xhr.status,
                        statusText: xhr.statusText,
                        text: xhr.responseText
                    });
                };
                
                xhr.onerror = function() {
                    // Network error occurred
                    debugResponse.textContent = `Error: Network request failed
Status: ${xhr.status}
Status Text: ${xhr.statusText}
Response: ${xhr.responseText}

This could be due to:
1. Network connectivity issues
2. Server is unreachable
3. Connection was terminated by the server
4. CORS issues`;

                    reject({
                        status: xhr.status,
                        statusText: xhr.statusText,
                        message: xhr.responseText || 'Network request failed',
                        responseText: xhr.responseText
                    });
                };
                
                xhr.open('POST', '/upload', true);
                xhr.send(formData);
            });
            
            // Wait for upload to complete
            const response = await uploadPromise;
            
            // Debug info for response
            debugResponse.textContent = `Status: ${response.status} ${response.statusText}
Response body: ${response.text}`;
            
            // Update status based on server response
            if (response.status >= 200 && response.status < 300) {
                statusDiv.innerHTML = '<span style="color: green;">✓</span> Upload successful';
                form.reset();
                document.getElementById('file-name').textContent = 'No file selected';
            } else {
                // Display the actual server error message
                let errorMessage = response.text || response.statusText || 'Unknown error';
                
                statusDiv.innerHTML = `<span style="color: red;">✗</span> Upload failed: ${errorMessage}`;
                
                // Show debug panel automatically for server errors
                if (debugToggle && debugPanel.classList.contains('hidden')) {
                    debugToggle.checked = true;
                    debugPanel.classList.remove('hidden');
                    
                    // Update toggle UI
                    const track = document.getElementById('toggle-track');
                    const thumb = document.getElementById('toggle-thumb');
                    track.classList.add('bg-blue-500');
                    track.classList.remove('bg-gray-300');
                    thumb.classList.add('transform', 'translate-x-4');
                }
            }
        } catch (error) {
            let errorMessage = error.message || 'Unknown error';
            
            // If we have a response text from the server, use that
            if (error.responseText) {
                errorMessage = error.responseText;
            }
            
            statusDiv.innerHTML = `<span style="color: red;">✗</span> Upload error: ${errorMessage}`;
            
            // Show debug panel automatically for errors
            if (debugToggle && debugPanel.classList.contains('hidden')) {
                debugToggle.checked = true;
                debugPanel.classList.remove('hidden');
                
                // Update toggle UI
                const track = document.getElementById('toggle-track');
                const thumb = document.getElementById('toggle-thumb');
                track.classList.add('bg-blue-500');
                track.classList.remove('bg-gray-300');
                thumb.classList.add('transform', 'translate-x-4');
            }
        }
    });
});