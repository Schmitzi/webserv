document.addEventListener('DOMContentLoaded', function() {
    console.log('Script loaded successfully!');
    
    // Simple test to verify JavaScript is working
    let testElement = document.createElement('div');
    testElement.textContent = 'JavaScript is working!';
    testElement.style.color = 'green';
    testElement.style.fontWeight = 'bold';
    
    document.body.appendChild(testElement);
});