document.addEventListener('DOMContentLoaded', function() {
    const blocks = document.querySelectorAll('.block');
    const dropZone = document.getElementById('drop-zone');
    const customInput = document.getElementById('custom-input');
    const addCustomBtn = document.getElementById('add-custom');
    const clearBtn = document.getElementById('clear-btn');
    const sendBtn = document.getElementById('send-btn');
    const requestOutput = document.getElementById('request-output');
    const responseOutput = document.getElementById('response-output');
    
    let draggedElement = null;
    
    // Initialize - remove placeholder text when blocks are added
    function initDropZone() {
        if (dropZone.children.length === 1 && 
            dropZone.children[0].style && 
            dropZone.children[0].style.color === 'rgb(170, 170, 170)') {
            // It's our placeholder text
            return false;
        }
        return true;
    }
    
    // Add event listeners to blocks
    blocks.forEach(block => {
        block.addEventListener('dragstart', function(e) {
            draggedElement = this;
            this.classList.add('dragging');
            e.dataTransfer.setData('text/plain', this.textContent);
        });
        
        block.addEventListener('dragend', function() {
            this.classList.remove('dragging');
        });
    });
    
    // Add custom block
    addCustomBtn.addEventListener('click', function() {
        if (customInput.value.trim() !== '') {
            const customBlock = document.createElement('div');
            customBlock.className = 'block custom-block';
            customBlock.setAttribute('draggable', true);
            customBlock.textContent = customInput.value.trim();
            
            customBlock.addEventListener('dragstart', function(e) {
                draggedElement = this;
                this.classList.add('dragging');
                e.dataTransfer.setData('text/plain', this.textContent);
            });
            
            customBlock.addEventListener('dragend', function() {
                this.classList.remove('dragging');
            });
            
            // Add to blocks container
            document.querySelector('.blocks-container').appendChild(customBlock);
            customInput.value = '';
        }
    });
    
    // Drop zone events
    dropZone.addEventListener('dragover', function(e) {
        e.preventDefault();
        this.style.backgroundColor = '#f0f0f0';
    });
    
    dropZone.addEventListener('dragleave', function() {
        this.style.backgroundColor = '#fff';
    });
    
    dropZone.addEventListener('drop', function(e) {
        e.preventDefault();
        this.style.backgroundColor = '#fff';
        
        // Clear placeholder text if this is the first block
        if (!initDropZone()) {
            dropZone.innerHTML = '';
        }
        
        // Create a new block in the drop zone
        const blockText = e.dataTransfer.getData('text/plain');
        const newBlock = document.createElement('div');
        
        if (blockText === 'GET' || blockText === 'POST' || blockText === 'DELETE') {
            newBlock.className = 'block method-block';
        } else if (blockText === 'HTTP/1.1') {
            newBlock.className = 'block version-block';
        } else {
            newBlock.className = 'block custom-block';
        }
        
        newBlock.setAttribute('draggable', true);
        newBlock.textContent = blockText;
        
        // Add remove button
        const removeBtn = document.createElement('button');
        removeBtn.className = 'remove-btn';
        removeBtn.textContent = '×';
        removeBtn.addEventListener('click', function() {
            newBlock.remove();
            updateRequestOutput();
            
            // Restore placeholder if empty
            if (dropZone.children.length === 0) {
                dropZone.innerHTML = '<div style="color: #aaa; text-align: center; width: 100%;">Drag blocks here to build your request</div>';
            }
        });
        
        newBlock.appendChild(removeBtn);
        
        // Add drag events to the new block
        newBlock.addEventListener('dragstart', function(e) {
            draggedElement = this;
            this.classList.add('dragging');
            e.dataTransfer.setData('text/plain', this.textContent.replace('×', '').trim());
        });
        
        newBlock.addEventListener('dragend', function() {
            this.classList.remove('dragging');
        });
        
        this.appendChild(newBlock);
        updateRequestOutput();
    });
    
    // Clear button
    clearBtn.addEventListener('click', function() {
        dropZone.innerHTML = '<div style="color: #aaa; text-align: center; width: 100%;">Drag blocks here to build your request</div>';
        updateRequestOutput();
        responseOutput.textContent = '# Server response will appear here';
    });
    
    // Update request output
    function updateRequestOutput() {
        if (!initDropZone()) {
            requestOutput.textContent = '# HTTP request will appear here';
            return;
        }
        
        let requestText = '';
        
        // Extract text content without the remove button
        dropZone.childNodes.forEach(child => {
            if (child.nodeType === Node.ELEMENT_NODE && child.classList.contains('block')) {
                requestText += child.textContent.replace('×', '').trim() + ' ';
            }
        });
        
        requestOutput.textContent = requestText.trim();
    }
    
    // Send request
    sendBtn.addEventListener('click', async function() {
        if (!initDropZone()) {
            alert('Please add some blocks to create a request.');
            return;
        }
        
        const requestText = requestOutput.textContent;
        responseOutput.textContent = 'Sending request...';
        
        // Parse the request text to extract method and path
        const parts = requestText.split(' ');
        let method = 'GET'; // Default method
        let path = '/';     // Default path
        let hasQueryParams = false;
        
        // Find method
        if (['GET', 'POST', 'DELETE'].includes(parts[0])) {
            method = parts[0];
        }
        
        // Find path (look for something that starts with / or is not HTTP/1.1)
        for (let i = 1; i < parts.length; i++) {
            if (parts[i].startsWith('/')) {
                path = parts[i];
                break;
            } else if (parts[i] !== 'HTTP/1.1') {
                // If it doesn't start with /, but isn't HTTP/1.1, assume it's a path
                if (!parts[i].startsWith('/')) {
                    path = '/' + parts[i];
                } else {
                    path = parts[i];
                }
                break;
            }
        }
        
        // Check if we have query parameters in the request
        if (path.includes('?')) {
            hasQueryParams = true;
        }
        
        // Format the request for display
        const fullRequest = `${method} ${path} HTTP/1.1
Host: ${window.location.host}
User-Agent: WebServ-RequestBuilder
Accept: */*
${method === 'POST' ? 'Content-Type: application/x-www-form-urlencoded\n' : ''}
`;
        
        // Display the full request that will be sent
        requestOutput.textContent = fullRequest;
        
        try {
            console.log("Sending request to:", path);
            let fetchOptions = {
                method: method,
                headers: {
                    'Accept': '*/*',
                    'User-Agent': 'WebServ-RequestBuilder'
                }
            };
            
            // For POST requests, we need to set up the body correctly
            if (method === 'POST') {
                // If we have query parameters in the URL, use them as the body
                if (hasQueryParams) {
                    const queryString = path.split('?')[1];
                    path = path.split('?')[0]; // Remove query from path for fetch
                    fetchOptions.headers['Content-Type'] = 'application/x-www-form-urlencoded';
                    fetchOptions.body = queryString;
                } else {
                    // Make sure we at least send an empty body
                    fetchOptions.headers['Content-Type'] = 'application/x-www-form-urlencoded';
                    fetchOptions.body = '';
                }
            }
            
            // Send the request
            const response = await fetch(path, fetchOptions);
            
            console.log("Response received:", response.status, response.statusText);
            
            // Create status line
            let responseText = `HTTP/1.1 ${response.status} ${response.statusText}\n`;
            
            // Add all headers
            console.log("Response headers:");
            response.headers.forEach((value, name) => {
                console.log(`${name}: ${value}`);
                responseText += `${name}: ${value}\n`;
            });
            
            // Add empty line to separate headers from body
            responseText += '\n';
            
            // Add body
            const body = await response.text();
            console.log("Response body:", body);
			
			// Render HTML in iframe
			const iframe = document.getElementById('rendered-output');
			const blob = new Blob([body], { type: 'text/html' });
			const blobUrl = URL.createObjectURL(blob);
			// iframe.src = blobUrl;
            iframe.src = path;
            // Display full response
            responseOutput.textContent = responseText + body;
        } catch (error) {
            console.error("Fetch error:", error);
            responseOutput.textContent = `Error: ${error.message}`;
        }
    });
});