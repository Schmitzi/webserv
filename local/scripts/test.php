#!/usr/bin/php
<?php
// Set content type header for CGI response
header("Content-Type: text/html");

// Get request information from environment variables
$requestMethod = $_SERVER['REQUEST_METHOD'] ?? 'Unknown';
$queryString = $_SERVER['QUERY_STRING'] ?? '';
$contentType = $_SERVER['CONTENT_TYPE'] ?? '';
$contentLength = $_SERVER['CONTENT_LENGTH'] ?? 0;

// Read POST data if any
$postData = '';
if ($contentLength > 0) {
    $postData = file_get_contents("php://input");
}

// Output the response
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head>\n";
echo "    <title>Hello World CGI Script</title>\n";
echo "</head>\n";
echo "<body>\n";
echo "    <h1>Hello World from PHP!</h1>\n";
echo "    <p>This is a CGI response from a PHP script.</p>\n";
echo "    <h2>Request Information:</h2>\n";
echo "    <ul>\n";
echo "        <li>Request Method: $requestMethod</li>\n";
echo "        <li>Query String: $queryString</li>\n";
echo "        <li>Content Type: $contentType</li>\n";
echo "        <li>Content Length: $contentLength</li>\n";
echo "    </ul>\n";

if (!empty($postData)) {
    echo "    <h2>POST Data:</h2>\n";
    echo "    <pre>" . htmlspecialchars($postData) . "</pre>\n";
}

echo "</body>\n";
echo "</html>";
?>