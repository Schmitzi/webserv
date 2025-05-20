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
        
        // Display file size warning for large files
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
        
        // DIRECT SIZE CHECK - Very important for WebServ which has limited file size support
        if (fileSizeMB > 50) { // Adjust this threshold based on your server limits
            statusDiv.innerHTML = `<span style="color: red;">✗</span> Error: File too large (${fileSizeStr})
<br><br>
Your file exceeds the server's size limit. The maximum file size for upload is likely around 1-10MB.
<br><br>
To upload larger files, please:
<br>
1. Use a smaller file, or<br>
2. Contact the server administrator to increase the limit`;
            
            debugResponse.textContent = `CLIENT ERROR: File size (${fileSizeStr}) exceeds server limit.
            
The server's client_max_body_size setting in the configuration file is likely too small for this file.
Based on the file size, you would need to increase this setting to at least ${Math.ceil(fileSizeGB + 1)}G in your server config.`;

            // Show debug panel automatically
            if (debugToggle && !debugPanel.classList.contains('hidden')) {
                debugToggle.checked = true;
            }
            return;
        }
        
        // Create FormData object to send the file
        const formData = new FormData();
        formData.append('file', file);
        
        statusDiv.textContent = 'Uploading...';
        
        try {
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
                    resolve({
                        status: xhr.status,
                        statusText: xhr.statusText,
                        text: xhr.responseText
                    });
                };
                
                xhr.onerror = function() {
                    // If there's a network error and the file is large, provide a specific message
                    debugResponse.textContent = `Error: Network request failed
Status: N/A
Status Text: Network Error

This is most likely because:
1. The file (${fileSizeStr}) exceeds the server's maximum allowed size
2. The server closed the connection without sending a proper HTTP response
3. The server's 'client_max_body_size' setting is too small for this file`;

                    reject({
                        status: 413, // Assume 413 for network errors with large files
                        statusText: 'Payload Too Large (Connection Terminated)',
                        message: `File size (${fileSizeStr}) likely exceeds server limit`
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
            
            // Update status
            if (response.status >= 200 && response.status < 300) {
                statusDiv.innerHTML = '<span style="color: green;">✓</span> Upload successful';
                form.reset();
            } else {
                const errorMsg = response.status === 413 
                    ? `File size (${fileSizeStr}) exceeds server limit` 
                    : `Error ${response.status}: ${response.statusText}`;
                
                statusDiv.innerHTML = `<span style="color: red;">✗</span> Upload failed: ${errorMsg}`;
            }
        } catch (error) {
            // For network errors that are likely size-related
            if (error.status === 413 || fileSizeMB > 10) {
                statusDiv.innerHTML = `<span style="color: red;">✗</span> Upload failed: ${error.message}
<br><br>
This is likely because your file (${fileSizeStr}) exceeds the server's size limit.
<br><br>
Please try with a smaller file or contact the server administrator.`;
            } else {
                statusDiv.innerHTML = `<span style="color: red;">✗</span> Upload error: ${error.message || 'Unknown error'}`;
            }
            
            // Show debug panel automatically for errors
            if (debugToggle && debugPanel.classList.contains('hidden')) {
                debugToggle.checked = true;
                debugPanel.classList.remove('hidden');
            }
        }
    });
});