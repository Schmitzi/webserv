document.addEventListener('DOMContentLoaded', () => {
    const dropZone = document.getElementById('drop-zone');
    const methodBlocks = document.querySelectorAll('.method-block');
    const versionBlocks = document.querySelectorAll('.version-block');
    const customInput = document.getElementById('custom-input');
    const addCustomBtn = document.getElementById('add-custom');
    const clearBtn = document.getElementById('clear-btn');
    const requestOutput = document.getElementById('request-output');

    // Drag and drop functionality
    methodBlocks.forEach(block => {
        block.addEventListener('dragstart', dragStart);
    });

    versionBlocks.forEach(block => {
        block.addEventListener('dragstart', dragStart);
    });

    dropZone.addEventListener('dragover', dragOver);
    dropZone.addEventListener('drop', drop);

    function dragStart(e) {
        e.dataTransfer.setData('text/plain', e.target.textContent);
        e.dataTransfer.effectAllowed = 'copy';
    }

    function dragOver(e) {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
    }

    function drop(e) {
        e.preventDefault();
        const blockText = e.dataTransfer.getData('text/plain');
        
        createBlock(blockText, 'bg-blue-500');
    }

    // Create block function
    function createBlock(text, bgClass = 'bg-green-500') {
        // Create a new block element
        const blockElement = document.createElement('div');
        blockElement.textContent = text;
        blockElement.classList.add('block', bgClass, 'text-white', 'p-2', 'rounded', 'm-1', 'inline-block', 'flex', 'items-center');
        
        // Add close button to the block
        const closeBtn = document.createElement('span');
        closeBtn.innerHTML = '&times;';
        closeBtn.classList.add('ml-2', 'cursor-pointer', 'text-white', 'hover:text-red-300');
        closeBtn.addEventListener('click', () => {
            blockElement.remove();
            updateRequestOutput();
        });
        
        blockElement.appendChild(closeBtn);

        // Clear existing content if it's the default text
        if (dropZone.children.length === 1 && dropZone.children[0].classList.contains('text-gray-500')) {
            dropZone.innerHTML = '';
        }

        // Append the new block
        dropZone.appendChild(blockElement);
        
        // Update request output
        updateRequestOutput();
    }

    // Custom block functionality
    addCustomBtn.addEventListener('click', () => {
        const customText = customInput.value.trim();
        if (customText) {
            createBlock(customText);
            
            // Clear input
            customInput.value = '';
        }
    });

    // Support Enter key for adding custom block
    customInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            const customText = customInput.value.trim();
            if (customText) {
                createBlock(customText);
                customInput.value = '';
            }
        }
    });

    // Clear all blocks
    clearBtn.addEventListener('click', () => {
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

        // Build request based on block order
        const requestParts = Array.from(blocks).map(block => 
            block.textContent.replace('×', '').trim()
        );

        requestOutput.textContent = requestParts.join(' ');
    }

    // Send request 
    const sendBtn = document.getElementById('send-btn');
    sendBtn.addEventListener('click', async () => {
        const request = requestOutput.textContent;
        const responseOutput = document.getElementById('response-output');
        
        try {
            // Parse the request to extract method and path
            const [method, path, version] = request.split(' ');
            
            // Perform the fetch request
            const response = await fetch(path || '/', {
                method: method || 'GET',
                headers: {
                    'Content-Type': 'application/json'
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
                bodyText = await response.text();
            } catch (error) {
                bodyText = 'Unable to read response body';
            }
            
            // Construct full response text with preserved formatting
            const fullResponseText = `HTTP ${status} ${statusText}\n\nHeaders:\n${headersText}\n\nBody:\n${bodyText}`;
            
            // Display full response details
            responseOutput.textContent = fullResponseText;
            
            // Optional: add syntax highlighting or pre-formatting
            responseOutput.style.whiteSpace = 'pre-wrap';
            responseOutput.style.fontFamily = 'monospace';
        } catch (error) {
            // Handle any network or fetch errors
            responseOutput.textContent = `Error sending request: ${error.message}`;
        }
    });
});