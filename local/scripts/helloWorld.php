<?php
/**
 * Simple PHP script to post "Hello World" to a server
 * 
 * Usage: 
 * 1. Replace 'https://your-server-url.com/endpoint' with your actual server URL
 * 2. Run the script with PHP: php post_hello_world.php
 */

// Server URL to post to
$serverUrl = 'http://localhost:8080';

// Data to send
$postData = [
    'message' => 'Hello World'
];

// Initialize cURL session
$ch = curl_init($serverUrl);

// Set cURL options
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_POST, true);
curl_setopt($ch, CURLOPT_POSTFIELDS, $postData);
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'Content-Type: application/x-www-form-urlencoded'
]);

// Execute the request
$response = curl_exec($ch);

// Check for errors
if(curl_errno($ch)) {
    echo 'Error: ' . curl_error($ch) . "\n";
} else {
    // Output the response
    echo "Server Response:\n";
    echo $response . "\n";
}

// Close cURL session
curl_close($ch);
?>