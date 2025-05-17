<?php
header("Content-Type: text/html");
echo "<html><body>";
echo "<h1>PATH_INFO Test</h1>";
echo "<p>PATH_INFO: " . htmlspecialchars($_SERVER['PATH_INFO'] ?? 'None') . "</p>";

if (!empty($_SERVER['PATH_INFO'])) {
    $segments = explode('/', trim($_SERVER['PATH_INFO'], '/'));
    echo "<p>Path segments:</p><ul>";
    foreach ($segments as $segment) {
        if (!empty($segment)) {
            echo "<li>" . htmlspecialchars($segment) . "</li>";
        }
    }
    echo "</ul>";
}

echo "<h2>All Server Variables</h2>";
echo "<pre>";
print_r($_SERVER);
echo "</pre>";
echo "</body></html>";
?>