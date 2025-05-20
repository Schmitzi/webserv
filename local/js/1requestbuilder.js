document.addEventListener('DOMContentLoaded', function() {
    // Core elements
    const dropZone = document.getElementById('drop-zone');
    const customInput = document.getElementById('custom-input');
    const addCustomBtn = document.getElementById('add-custom');
    const clearBtn = document.getElementById('clear-btn');
    const sendBtn = document.getElementById('send-btn');
    const requestOutput = document.getElementById('request-output');
    const responseOutput = document.getElementById('response-output');

    // Validate all required elements exist
    if (!dropZone || !customInput || !addCustomBtn || !clearBtn || !requestOutput) {
        console.error('One or more required elements are missing');
        return;
    }

    // Debugging function
    function log(message) {
        console.log(`[Request Builder] ${message}`);
    }

    // Create a block
    function createBlock(text, bgClass = 'bg-green-500') {
        log(`Creating block: ${text}`);

        // Trim and validate text
        text = text.trim();
        if (!text) {
            log('Attempted to create empty block');
            return;
        }

        // Create block element
        const blockElement = document.createElement('div');
        blockElement.className = `block ${bgClass} text-white p-2 rounded m-1 inline-flex items-center relative group`;
        
        // Block text
        const textSpan = document.createElement('span');
        textSpan.textContent = text;
        blockElement.appendChild(textSpan);

        // Close button
        const closeBtn = document.createElement('button');
        closeBtn.innerHTML = '&times;';
        closeBtn.className = `ml-2 text-white opacity-0 group-hover:opacity-100 absolute -top-2 -right-2 bg-red-500 rounded-full w-5 h-5 flex items-center justify-center text-xs`;
        
        closeBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            blockElement.remove();
            updateRequestOutput();
        });

        blockElement.appendChild(closeBtn);

        // Clear placeholder if needed
        if (dropZone.children.length === 1 && dropZone.children[0].classList.contains('text-gray-500')) {
            dropZone.innerHTML = '';
        }

        // Add block to drop zone
        dropZone.appendChild(blockElement);
        updateRequestOutput();
    }

    // Add custom block
    function addCustomBlock() {
        const customText = customInput.value.trim();
        if (customText) {
            log(`Adding custom block: ${customText}`);
            createBlock(customText);
            customInput.value = ''; // Clear input
            customInput.focus(); // Return focus to input
        } else {
            log('No text entered');
        }
    }

    // Event Listeners
    addCustomBtn.addEventListener('click', addCustomBlock);

    // Support Enter key in input field
    customInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            log('Enter key pressed');
            addCustomBlock();
        }
    });

    // Clear all blocks
    clearBtn.addEventListener('click', () => {
        log('Clearing all blocks');
        dropZone.innerHTML = '<div class="text-gray-500 text-center">Drag blocks here to build your request</div>';
        updateRequestOutput();
    });

    // Update request output
    function updateRequestOutput() {
        const blocks = dropZone.querySelectorAll('.block');
        
        if (blocks.length === 0) {
            requestOutput.textContent = '# HTTP request will appear here';
            return;
        }

        // Build request from blocks
        const requestParts = Array.from(blocks)
            .map(block => block.textContent.replace('×', '').trim());

        requestOutput.textContent = requestParts.join(' ');
        log(`Request output updated: ${requestOutput.textContent}`);
    }

    // Drag and drop functionality
    const methodBlocks = document.querySelectorAll('.method-block');
    const versionBlocks = document.querySelectorAll('.version-block');

    // Add drag start to predefined blocks
    methodBlocks.forEach(block => {
        block.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('text/plain', e.target.textContent);
        });
    });

    versionBlocks.forEach(block => {
        block.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('text/plain', e.target.textContent);
        });
    });

    // Drop zone event listeners
    dropZone.addEventListener('dragover', (e) => {
        e.preventDefault();
    });

    dropZone.addEventListener('drop', (e) => {
        e.preventDefault();
        const blockText = e.dataTransfer.getData('text/plain');
        
        // Determine background class
        const bgClass = methodBlocks.length > 0 && 
            Array.from(methodBlocks).some(block => block.textContent === blockText) 
            ? 'bg-blue-500' 
            : (versionBlocks.length > 0 && 
               Array.from(versionBlocks).some(block => block.textContent === blockText) 
               ? 'bg-purple-500' 
               : 'bg-green-500');
        
        createBlock(blockText, bgClass);
    });

    // Send request (basic implementation)
    sendBtn.addEventListener('click', async () => {
        const request = requestOutput.textContent.trim();
        
        if (request === '# HTTP request will appear here' || request === '') {
            responseOutput.textContent = 'Error: No request created. Please add blocks.';
            return;
        }

        try {
            // Parse request
            const parts = request.split(' ');
            const method = parts[0] || 'GET';
            const path = parts[1] || '/';

            // Perform fetch
            const response = await fetch(path, {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                    'Accept': 'application/json'
                }
            });
            
            // Get response details
            const status = response.status;
            const statusText = response.statusText;
            
            // Collect headers
            const headersText = Array.from(response.headers.entries())
                .map(([key, value]) => `${key}: ${value}`)
                .join('\n');
            
            // Try to parse response body
            let bodyText;
            try {
                const jsonBody = await response.json();
                bodyText = JSON.stringify(jsonBody, null, 2);
            } catch {
                try {
                    bodyText = await response.text();
                } catch {
                    bodyText = 'Unable to read response body';
                }
            }
            
            // Display response
            const fullResponseText = `HTTP ${status} ${statusText}\n\nHeaders:\n${headersText}\n\nBody:\n${bodyText}`;
            responseOutput.textContent = fullResponseText;
            responseOutput.style.whiteSpace = 'pre-wrap';
            responseOutput.style.fontFamily = 'monospace';
        } catch (error) {
            responseOutput.textContent = `Error sending request:\n${error.message}`;
        }
    });

    // Initial log
    log('Request Builder script initialized');
});