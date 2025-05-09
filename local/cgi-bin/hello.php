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
echo "    <style>body { font-family: Arial, sans-serif; margin: 20px; }</style>\n";
echo "    <link rel=\icon\" href=\"/images/favicon.ico\" type=\"image/x-icon\">\n";
echo "</head>\n";
echo "<body>\n";
echo "    <h1>Hello World from PHP!</h1>\n";
echo "    <div><a href=\"../cgi.html\">Back</a></div>\n";
echo "    <p>This is a CGI response from a PHP script.</p>\n";
echo "    <h2>Request Information:</h2>\n";
echo "    <ul>\n";
echo "        <li><strong>Request Method:</strong> $requestMethod</li>\n";
echo "        <li><strong>Query String:</strong> $queryString</li>\n";
echo "        <li><strong>Content Type:</strong> $contentType</li>\n";
echo "        <li><strong>Content Length:</strong> $contentLength</li>\n";
echo "    </ul>\n";

// Parse and display GET parameters
if (!empty($_GET)) {
    echo "    <h2>GET Parameters:</h2>\n";
    echo "    <ul>\n";
    foreach ($_GET as $key => $value) {
        echo "        <li><strong>$key:</strong> $value</li>\n";
    }
    echo "    </ul>\n";
}

// Parse and display POST parameters
if (!empty($_POST)) {
    echo "    <h2>POST Parameters:</h2>\n";
    echo "    <ul>\n";
    foreach ($_POST as $key => $value) {
        echo "        <li><strong>$key:</strong> $value</li>\n";
    }
    echo "    </ul>\n";
    
    // Special handling for message parameter
    if (isset($_POST['message'])) {
        echo "    <h3>Your message: " . htmlspecialchars($_POST['message']) . "</h3>\n";
    }
}

// Display raw POST data if available
if (!empty($postData)) {
    echo "    <h2>Raw POST Data:</h2>\n";
    echo "    <pre>" . htmlspecialchars($postData) . "</pre>\n";
}

echo "</body>\n";
echo "</html>";
?>