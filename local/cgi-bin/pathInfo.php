<?php
// path_demo.php - Simple PATH_INFO demonstration
header("Content-Type: text/html");

// Get the important server variables
$request_uri = $_SERVER['REQUEST_URI'] ?? '';
$script_name = $_SERVER['SCRIPT_NAME'] ?? '';
$path_info = $_SERVER['PATH_INFO'] ?? '';
$query_string = $_SERVER['QUERY_STRING'] ?? '';

// Extract PATH_INFO segments if available
$segments = [];
if (!empty($path_info)) {
    $segments = explode('/', trim($path_info, '/'));
    $segments = array_filter($segments); // Remove empty segments
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>PATH_INFO Demo</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
        }
        h1 {
            border-bottom: 2px solid #eaeaea;
            padding-bottom: 10px;
            color: #333;
        }
        .info-box {
            background-color: #f9f9f9;
            border-left: 4px solid #2563eb;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 4px;
        }
        .segment-box {
            background-color: #f0f7ff;
            padding: 15px;
            margin-bottom: 10px;
            border-radius: 4px;
        }
        .request-details dt {
            font-weight: bold;
            margin-top: 10px;
        }
        .request-details dd {
            margin-left: 20px;
            background: #f0f0f0;
            padding: 5px 10px;
            border-radius: 4px;
            font-family: monospace;
            overflow-wrap: break-word;
        }
        .example-usage {
            background-color: #f0fff4;
            border-left: 4px solid #10b981;
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
        }
        .example-usage code {
            display: block;
            background: #e6ffec;
            padding: 10px;
            margin: 5px 0;
            border-radius: 4px;
            font-family: monospace;
        }
    </style>
</head>
<body>
    <h1>PATH_INFO Demonstration</h1>
    
    <div class="info-box">
        <h2>Request Details</h2>
        <dl class="request-details">
            <dt>Full Request URI:</dt>
            <dd><?php echo htmlspecialchars($request_uri); ?></dd>
            
            <dt>Script Name:</dt>
            <dd><?php echo htmlspecialchars($script_name); ?></dd>
            
            <dt>PATH_INFO:</dt>
            <dd><?php echo !empty($path_info) ? htmlspecialchars($path_info) : "<em>(none)</em>"; ?></dd>
            
            <dt>Query String:</dt>
            <dd><?php echo !empty($query_string) ? htmlspecialchars($query_string) : "<em>(none)</em>"; ?></dd>
        </dl>
    </div>
    
    <?php if (!empty($segments)): ?>
        <h2>PATH_INFO Segments</h2>
        
        <?php foreach ($segments as $index => $segment): ?>
            <div class="segment-box">
                <h3>Segment <?php echo $index + 1; ?>: <?php echo htmlspecialchars($segment); ?></h3>
                
                <?php if ($index == 0): ?>
                    <p>This typically represents the main <strong>resource</strong> being requested.</p>
                <?php elseif ($index == 1): ?>
                    <p>This often represents an <strong>identifier</strong> or <strong>action</strong> for the resource.</p>
                <?php elseif ($index == 2): ?>
                    <p>This could represent a <strong>sub-resource</strong> or additional parameter.</p>
                <?php endif; ?>
            </div>
        <?php endforeach; ?>
        
        <?php if (count($segments) >= 2): ?>
            <div class="example-usage">
                <h3>Example Usage in a RESTful API</h3>
                
                <?php if ($segments[0] == 'users' && isset($segments[1])): ?>
                    <?php if (is_numeric($segments[1])): ?>
                        <p>This URL pattern would typically retrieve user information:</p>
                        <code>GET /users/<?php echo htmlspecialchars($segments[1]); ?></code>
                        <p>In a real application, this might fetch user #<?php echo htmlspecialchars($segments[1]); ?> from a database.</p>
                    <?php else: ?>
                        <p>This URL pattern could represent a user action:</p>
                        <code>POST /users/<?php echo htmlspecialchars($segments[1]); ?></code>
                        <p>For example, "<strong><?php echo htmlspecialchars($segments[1]); ?></strong>" might be an action like "create", "update", etc.</p>
                    <?php endif; ?>
                <?php elseif ($segments[0] == 'products' && isset($segments[1])): ?>
                    <?php if (isset($segments[2])): ?>
                        <p>This hierarchical URL pattern could represent nested resources:</p>
                        <code>GET /products/<?php echo htmlspecialchars($segments[1]); ?>/<?php echo htmlspecialchars($segments[2]); ?></code>
                        <p>For example, this might fetch <?php echo htmlspecialchars($segments[2]); ?> in the <?php echo htmlspecialchars($segments[1]); ?> category.</p>
                    <?php else: ?>
                        <p>This URL pattern would typically retrieve product information:</p>
                        <code>GET /products/<?php echo htmlspecialchars($segments[1]); ?></code>
                        <p>In a real application, this might fetch details about a specific product.</p>
                    <?php endif; ?>
                <?php else: ?>
                    <p>In a RESTful API, this URL pattern might represent:</p>
                    <code>GET /<?php echo htmlspecialchars($segments[0]); ?>/<?php echo htmlspecialchars($segments[1]); ?></code>
                    <p>Where "<strong><?php echo htmlspecialchars($segments[0]); ?></strong>" is the resource type and "<strong><?php echo htmlspecialchars($segments[1]); ?></strong>" is the identifier.</p>
                <?php endif; ?>
            </div>
        <?php endif; ?>
    <?php else: ?>
        <div class="info-box">
            <h3>No PATH_INFO Segments</h3>
            <p>This request doesn't contain any PATH_INFO segments. Try appending something to the URL after the script name, like:</p>
            <p><code><?php echo htmlspecialchars($script_name); ?>/users/123</code></p>
        </div>
    <?php endif; ?>
    
    <h2>Try Different Examples</h2>
    <ul>
        <li><a href="<?php echo htmlspecialchars(basename($script_name)); ?>">No PATH_INFO</a></li>
        <li><a href="<?php echo htmlspecialchars(basename($script_name)); ?>/users">Resource: /users</a></li>
        <li><a href="<?php echo htmlspecialchars(basename($script_name)); ?>/users/123">Resource with ID: /users/123</a></li>
        <li><a href="<?php echo htmlspecialchars(basename($script_name)); ?>/products/electronics/featured">Nested resources: /products/electronics/featured</a></li>
        <li><a href="<?php echo htmlspecialchars(basename($script_name)); ?>/search?q=example">With query string: /search?q=example</a></li>
    </ul>
</body>
</html>