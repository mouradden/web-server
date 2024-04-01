<?php
$file = fopen("form_data.txt", "a");
if (!$file) {
    die("Unable to open file");
}

// Write each POST field to the file
foreach ($_POST as $key => $value) {
    $written = fwrite($file, "$key: $value\n");
    if ($written === false) {
        die("Unable to write to file");
    }
}

// If there are any uploaded files, write their names and temporary paths to the file
foreach ($_FILES as $key => $value) {
    $written = fwrite($file, "$key: " . $value['name'] . " (temporary path: " . $value['tmp_name'] . ")\n");
    if ($written === false) {
        die("Unable to write to file");
    }
}

if (!fclose($file)) {
    die("Unable to close file");
}
?>