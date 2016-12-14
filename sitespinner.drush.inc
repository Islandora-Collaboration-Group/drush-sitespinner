<?php
/**
 * @file
 * sitespinner.drush.inc
 *
 * functions connected with commands that are used from the shell
 * will return 0 on succes and positive int on failure.
 *
 * functions that are used internally will do the opposite.
 */

/**
 * The default user of all the web sites
 * This will be the owner of the settings.php file created by the sitespinner
 * command.
 * We set this to the user who is running the drush command.
 */
define ('SITESPINNER_DEFAULT_USER', getenv('USER'));

/**
 * This will be the group of the settings.php file created by the sitespinner
 * command.
 * On my system the default apache user is www-data (ubuntu). If we set the files
 * directory to be owned by this group we insure that only apache can read and
 * write the files. Also if we set the group of the settings file to this group,
 * we insure that the server can read the settings file
 */
define ('SITESPINNER_DEFAULT_GROUP', '_www');

/**
 * Permissions for the settings.php file.
 * if we set the settings.php file to 740 we insure that only the
 * SITESPINNER_DEFAULT_USER and SITESPINNER_DEFAULT_GROUP can read the
 * settings.php file. User get full rights.
 */
define ('SITESPINNER_SETTINGS_PERMS', '740');

/**
 * Permissions for the files/ folder
 * if we set the files/ folder to 770 we insure that only the
 * SITESPINNER_DEFAULT_USER and SITESPINNER_DEFAULT_GROUP can read and write
 * to this folder
 */
define ('SITESPINNER_FILES_DIR_PERMS', '770');

/**
 * definitions for what to echo when exec commands execute well or with errors
 * or when simulating the commands
 */
define ('ERROR_PREPEND',    dt("Error:    Executing: "));
define ('NO_ERROR_PREPEND', dt("Ok:       Executing: "));
define ('SIMULATE_PREPEND', dt("Simulate: Executing: "));



// {{{ sitespinner_drush_command()

/**
 * Implements hook_drush_command().
 */
function sitespinner_drush_command() {
  $items = array();
  // The options we want to use in this script.
  $options['-f'] = 'Force. Do not halt on error';
  $options['-s'] = 'Silence. Less output';
  $options['--simulate'] = 'Use simulate when you want to see what will happen to your system. It will print the commands to be executed.';

  // The key in the $items array is the name of the command.
  $items['sitespinner'] = array(
    'description' => dt('create a drupal multi-site install. Will create the specified multi-site in the base /sites folder. E.g. yourmultisite will be placed in sites/www.base-site.net.yourmultisite'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    // The name of the function implementing your command.
    'callback' => 'sitespinner_sitespinner',
    // A short description of your command.
    'arguments' => array(
      'base-alias' => dt('The name of the site alias that will serve as the source template.'),
      'destination-alias' => dt('The drush site alias for the sub-site to create. Use the "domain" option if this site should be reachable on its own domain or subdomain.'),
    ),
    'options' => $options,
    'aliases' => array('ss'),
  );

  $items['sitespinner-delete'] = array(
    'description' => dt('Delete a site: the database and site directory for a given site alias'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    // The name of the function implementing your command.
    'callback' => 'sitespinner_sitespinner_delete',
    // A short description of your command.
    'arguments' => array(
      'alias' => dt('The name of the site alias that will be deleted.'),
    ),
    'aliases' => array('ssd'),
  );

  return $items;
}

// }}}
// {{{ sitespinner_drush_help()

/**
 * Implements hook_drush_help().
 */
function sitespinner_drush_help($section) {
  switch ($section) {
    case 'drush:sitespinner':
      return dt("drush sitespinner is a command that creates a new multi-site cloned from an existing base site. In detail, it: \n* Creates a new folder inside the /sites/ folder for the new site\n* Copies over the base site's :files directory.\n* Creates a new database and copies the data from the base site's database.\n* Creates a settings.php file.\n* Root and site-specific configuration overrides may be provided in the destination site alias.
\nThe command requires two drush site aliases as arguments: base site, and destination site. Therefore, before you can spin up a new site, you need to have created site aliases for both of these.
\nConsult the sample aliases file included with this command for complete instructions.");
      break;

    case 'drush:sitespinner-delete' :
      return dt('sitespinner-delete is a command that deletes the site directory and associated database corresponding to the provided site alias');
        break;
  }
}

// }}} sitespinner_drush_help ()
// {{{ sitespinner_sitespinner($site)

/**
 * function which will try to enable a sub-site to a drupal base site
 *
 * @param string $base
 *   The source alias. May be '@self'
 *
 * @param string $destination
 *   The destination alias.
 *
 * @return int
 *   1 on failure on 0 on success. This will send exit code to
 *   the shell.
 */
function sitespinner_sitespinner($base, $destination) {

  $force = drush_get_option('f');
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  // Verify base site alias.
  $base_site_alias_record = drush_sitealias_get_record($base);
  if (!$base_site_alias_record) {
    sitespinner_print(dt("'@alias' was not recognized as a valid source site alias. Exit. ", array('@alias' => $base)) . "\n");
    return 1;
  }
  // Verify destination site alias.
  $destination_site_alias_record = sitespinner_drush_sitealias_get_record($destination);
  if (!$destination_site_alias_record) {
    sitespinner_print(dt("'@alias' was not recognized as a valid destination site alias. Exit. ", array('@alias' => $destination)) . "\n");
    return 1;
  }

  $drupal_root = $destination_site_alias_record['root'];
  $parse_uri = explode('/', $destination_site_alias_record['uri']);
  $site = end($parse_uri);
  $multisite_root = $drupal_root . '/sites/' . str_replace('/', '.', $destination_site_alias_record['uri']);

  // Determine domain type (path, domain, or subdomain).
  $domain = 'path';
  if (empty($destination_site_alias_record['sitespinner-destination']['create-domain']['type']) || !(in_array($destination_site_alias_record['sitespinner-destination']['create-domain']['type'], array('path', 'domain', 'subdomain')))) {
    $types = array('path', 'domain', 'subdomain');
    $reply = drush_choice($types, 'Please indicate the domain type to use for this site by entering a number from the following list.', '!value');
    if ($reply === FALSE) {
      return 1;
    }
    $domain = $types[$reply];
    sitespinner_print(dt("\"@domain\" was chosen for the domain setting.\n", array('@domain' => $domain)));
  }

  // Test to see if we can sudo. If not, exit.
  $res = sitespinner_exec("sudo echo -e " . dt("Started enabling a sub-site"));
  if ($res) {
    sitespinner_print(dt("Could not sudo. Exit"));
    return 1;
  }

  // Try to create the database. If it exists, try to delete and re-create.
  $destination_db_spec = sitespinner_enable_db($destination_site_alias_record);
  if (!$destination_db_spec) {
    sitespinner_print(dt("Could not make database. Exit\n"));
    return 1;
  }

  // Create the multisite root with settings.php file.
  sitespinner_create_site($destination_site_alias_record);

  // Copy database and files directory from source.
  sitespinner_sql_sync($base, $destination);
  sitespinner_rsync_files($base, $destination);

  // Set destination variables using the destination site alias record.
  sitespinner_set_variables($destination);

  // Handle domain setting.
  switch ($domain) {
    case 'domain':
      // TODO vhost stuff.
      // sitespinner_create_virtualhost($site);
      break;

    case 'subdomain':
      // TODO vhost stuff.
      // sitespinner_create_virtualhost($site);
      break;

    default:
      // Create a to-self symlink using multisite name in the drupal root.
      sitespinner_exec(dt('sudo ln -s . @uri ', array('@uri' => $drupal_root . "/" . $site)));
      $default_user = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['default_user']) ?
        $destination_site_alias_record['sitespinner-destination']['server-environment']['default_user'] :
        SITESPINNER_DEFAULT_USER;
      $default_group = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['default_group']) ?
        $destination_site_alias_record['sitespinner-destination']['server-environment']['default_group'] :
        SITESPINNER_DEFAULT_GROUP;
      sitespinner_exec("sudo chown -h " . $default_user . ":" . $default_group . " " . $drupal_root . "/" . $site);

      break;

  }

  $msgs = array('');
  $msgs[] = dt('!sitepath and its drupal database have been created.', array('!sitepath' => $multisite_root));
  $msgs[] = '';
  $msgs[] = dt('Be sure to edit the filter-drupal.xml file on the Fedora server to fully enable this site.');
  $msgs[] = dt('Documentation: https://wiki.duraspace.org/pages/viewpage.action?pageId=69833569');
  $msgs[] = '';
  sitespinner_print($msgs);
  return 0;
}

// }}}
// {{{ sitespinner_sitespinner_delete()

function sitespinner_sitespinner_delete($alias) {

  $alias_record = drush_sitealias_get_record($alias);

  $readline_str = dt("Are you sure you want to delete the site at !uri?", array('!uri' => $alias_record['uri'])) . dt(" [y/N]? ");
  $confirm = strtoupper(sitespinner_readline($readline_str));

  // Check to see if the user wants to use existing database.
  if ($confirm != 'Y') {
    sitespinner_print(dt('Cancelled!') . "\n");
    return 0;
  }

  $res = sitespinner_exec("sudo echo -e " . dt("Started deleting a sub-site"));
  if ($res) {
    sitespinner_print(dt("Could not sudo. Exit"));
    return 1;
  }


  if (!empty($alias_record['databases']['default']['default'])) {
    $db_spec = $alias_record['databases']['default']['default'];
    // Use db_creator credentials, if provided.
    if (!empty($alias_record['sitespinner-destination']['db_creator'])) {
      $db_spec['username'] = $alias_record['sitespinner-destination']['db_creator']['username'];
      $db_spec['password'] = $alias_record['sitespinner-destination']['db_creator']['password'];
    }

    sitespinner_delete_db($db_spec);
  }

  $drupal_root = $alias_record['root'];
  $parse_uri = explode('/', $alias_record['uri']);
  $site = end($parse_uri);
  $multisite_root = $drupal_root . '/sites/' . str_replace('/', '.', $alias_record['uri']);
  $symlink = $drupal_root . '/' . $site;

  // Delete the site's files directory, if present.
  // TODO: how to ensure it's not in use by another site?!!
  if (!empty($alias_record['path-aliases']['%files']) && file_exists($alias_record['path-aliases']['%files'])) {
    sitespinner_exec(dt('rm -r !filespath', array('!filespath' => $alias_record['path-aliases']['%files'])));
  }

  // Delete the sites/[multisite-uri] directory.
  if (file_exists($multisite_root)) {
    sitespinner_exec(dt('rm -r !sitepath', array('!sitepath' => $multisite_root)));
  }
  else {
    sitespinner_print(dt('Attempting to delete !multisiteroot, but it does not exist. Skipping...', array('!multisiteroot' => $multisite_root)) . "\n");
  }

  // Delete the multisite path symlink at the drupal root, if present.
  if (file_exists($symlink)) {
    sitespinner_exec(dt('rm !link', array('!link' => $symlink)));
  }
  else {
    sitespinner_print(dt('Attempting to delete !symlink, but it does not exist. Skipping...', array('!symlink' => $symlink)) . "\n");
  }
  $msgs = array('');
  $msgs[] = dt('!sitepath and its drupal database have been deleted.', array('!sitepath' => $multisite_root));
  $msgs[] = '';
  $msgs[] = dt('Be sure to edit the filter-drupal.xml file on the Fedora server to remove this site.');
  $msgs[] = dt('Documentation: https://wiki.duraspace.org/pages/viewpage.action?pageId=69833569');
  $msgs[] = '';
  sitespinner_print($msgs);
}

// }}}
// {{{ sitespinner_exec()

/**
 * Any execution (exec) of a command is wrapped around the sitespinner_exec
 * command.
 * Then we can easily control when to halt on errors or when to force.
 * We can also control if the exec command shall be used in silence or
 * simulated.
 *
 * @param string $command
 *   the command to be executed
 *
 * @param int $force
 *   set in case we want the script to continue executing. This
 *   also silences the command. This param is meant as a way to test shell
 *   commands, e.g. testing if a database exists or not, without killing the
 *   script.
 *
 * @return int
 *   $result, the $result of the executed shell command 0 on success and
 *   positive int on failure. Depending on the $force flag it will stop
 *   executing the script on failure (positive int) and return the result.
 */
function sitespinner_exec($command, $force = 0) {
  // Get options.
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  // If we set the force when using sitespinner_exec
  // from inside these scripts, we will only use the result internally.
  if (!$force) {
    $force = drush_get_option('f');
  }
  else {
    $silent = 1;
  }

  $message = '';
  if ($simulate) {
    $message = SIMULATE_PREPEND;
    $message .= $command . "\n";
    sitespinner_print($message);
    return 0;
  }

  $output = $result = 0;
  // Exec command in silence. Store output of executed command in $output.
  exec($command . ' 2>&1', $output, $result);

  // ERROR executing command.
  if ($result) {
    $message .= ERROR_PREPEND;
    $message .= $command . "\n";
    if (empty($silent)) {
      sitespinner_print($message);
      sitespinner_print($output);
    }
    if (empty($force)) {
      return exit(1);
    }
  }

  // OK executing command.
  if (!$result) {
    $message .= NO_ERROR_PREPEND;
    $message .= $command . "\n";

    if (empty($silent)) {
      sitespinner_print($message);
      sitespinner_print($output);
    }
  }
  return $result;
}

// }}} sitespinner_exec()
// {{{ sitespinner_print($output)

/**
 * function for writing output to the screen. Or making a nice string
 * out of an array
 *
 * @param
 *    int $ret if a positive integer. The function will not print, but return
 *    content as a string instead
 * @param
 *   string $output the return string from sitespinner_exec commands to write out
 * @return
 *   string $buf optional. If $ret is set function will return a string
 */
function sitespinner_print($output, $ret = 0) {
  // exec uses a array to store output in
  $buf = '';
  if (is_array($output)) {
    foreach ($output as $line) {
      if (!$ret) {
        print($line) . "\n";
      }
      else {
        $buf .= $line . "\n";
      }
    }
    // else: just print the line
  }
  else {
    if (!$ret) {
      print($output);
    }
  }
  if ($ret) {
    return $buf;
  }
}

// }}}
// {{{ sitespinner_create_virtualhost($site)

/**
 * we assume your websites are arranged like this:
 *
 * /home/user/www/default_drupal_website/htdocs
 * /home/user/www/default_drupal_website/logs
 *
 * /home/user/www/new_drupal_website/logs
 * /home/user/www/new_drupal_website/logs/error.log
 * /home/user/www/new_drupal_website/logs/access.log
 *
 * Creates log files for a website
 *
 *
 * @param
 *   string $site the site to enable
 * @return
 *   void all error handling is being done in sitespinner_exec
 */
function sitespinner_create_virtualhost($site, $standalone = FALSE) {
  $settings = sitespinner_get_filenames($site);

  // create the log files
  sitespinner_exec("mkdir " . $settings['www_base_site']);
  sitespinner_exec("mkdir " . $settings['www_base_site_logs']);
  sitespinner_exec("touch " . $settings['www_base_site_access']);
  sitespinner_exec("touch " . $settings['www_base_site_error']);

  // search and replace in apache configuration
  sitespinner_exec("cp " . $settings['apache_default_conf'] . " /tmp/apache2.conf");
  $apache_default_string = sitespinner_get_file("/tmp/apache2.conf");
  $search = array("server_name", "document_root", "site_root", "allow_ip");
  $replace = array(
    $site,
    $settings['drupal_root'],
    $settings['www_base_site'],
    sitespinner_ALLOW_IP
  );
  $apache_default_string = str_replace($search, $replace, $apache_default_string);

  // write the file 
  sitespinner_write_settings("/tmp/apache2.conf", $apache_default_string);
  sitespinner_exec("sudo cp /tmp/apache2.conf " . $settings['apache_sites_avail']);

  // reload apache
  sitespinner_exec("sudo a2ensite $site");
  sitespinner_exec("sudo /etc/init.d/apache2 reload");
  sitespinner_update_etc_hosts($site);

}

// }}}
// {{{ sitespinner_create_site($site, $db_spec)

/**
 * Function for creating new drupal multi-site folder, settings file and files directory.
 *
 * @param string $destination_site_alias_record
 *   the alias of the drupal website to create
 */

// FIX: Maybe check if directory is writable and return 1 or 0.
function sitespinner_create_site($destination_site_alias_record) {

  $multisite_root = $destination_site_alias_record['root'] . '/sites/' . str_replace('/', '.', $destination_site_alias_record['uri']);
  sitespinner_exec("mkdir " . $multisite_root);
  $destination_settings_file_path = $multisite_root . "/settings.php";
  $drupal_root = $destination_site_alias_record['root'];
  if (!empty($destination_site_alias_record['settings_file_template'])) {
    $source_settings_file_path = $destination_site_alias_record['settings_file_template'];
    if (strpos('/', $source_settings_file_path) !== 0) {
      $source_settings_file_path = $drupal_root . "/" . $source_settings_file_path;
    }
  }
  else {
    $source_settings_file_path = $drupal_root . "/sites/default/default.settings.php";
  }
  sitespinner_exec("cp " . $source_settings_file_path . " " . $destination_settings_file_path);

  $settings_string = sitespinner_get_file($destination_settings_file_path);

  // Search and replace $databases and write new settings file.
  $database_settings_string = var_export($destination_site_alias_record['databases'], TRUE);
  $search = "\n\$databases = array();\n";
  $replace = "\n\$databases = " . $database_settings_string . ";\n";
  $settings_string = str_replace($search, $replace, $settings_string);
  sitespinner_write_settings($destination_settings_file_path, $settings_string);

  // Create and secure the site root directory. Use server environment settings from the destination alias file, if possible.
  $destination_files_directory = !empty($destination_site_alias_record['sitespinner-destination']['path-aliases']['%files']) ?
    $destination_site_alias_record['sitespinner-destination']['path-aliases']['%files'] :
    $multisite_root . '/files';
  $default_user = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['default_user']) ?
    $destination_site_alias_record['sitespinner-destination']['server-environment']['default_user'] :
    SITESPINNER_DEFAULT_USER;
  $default_group = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['default_group']) ?
    $destination_site_alias_record['sitespinner-destination']['server-environment']['default_group'] :
    SITESPINNER_DEFAULT_GROUP;
  $files_directory_permissions = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['files_directory_permissions']) ?
    $destination_site_alias_record['sitespinner-destination']['server-environment']['files_directory_permissions'] :
    SITESPINNER_FILES_DIR_PERMS;
  $setings_file_permissions = !empty($destination_site_alias_record['sitespinner-destination']['server-environment']['settings_file_permissions']) ?
    $destination_site_alias_record['sitespinner-destination']['server-environment']['settings_file_permissions'] :
    SITESPINNER_SETTINGS_PERMS;
  sitespinner_exec("sudo mkdir " . $destination_files_directory);
  sitespinner_exec("sudo chmod " . $files_directory_permissions . " " . $destination_files_directory);
  sitespinner_exec("sudo chown " . $default_user . ":" . $default_group . " " . $destination_files_directory);
  sitespinner_exec("sudo chmod " . $setings_file_permissions . " " . $destination_settings_file_path);
  sitespinner_exec("sudo chown " . $default_user . ":" . $default_group . " " . $destination_settings_file_path);
}

// }}}
// db and sql related functions 
// {{{ sitespinner_create_db($db_spec)

/**
 * function for creating a database. supporting mysql and postgres
 *
 * @param
 *   array $db_spec with information about the database to be created
 * @return
 *   int  1 on success and 0 on failure.
 *
 */
function sitespinner_create_db($db_spec) {
  drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sql.drush');
  return _drush_sql_create($db_spec);
}

// }}}
// {{{ sitespinner_delete_db($db_spec)

/**
 * function for deleting a database
 * @param
 *   array $db_spec with information about the database to be deleted
 * @return
 *   int 1 on success 0 on failure
 */
function sitespinner_delete_db($db_spec) {
  drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sql.drush');
  $success = _drush_sql_drop($db_spec);
  if ($success) {
    return 1;
  }
  else {
    sitespinner_print(dt('Could not delete database!'));
    return 0;
  }
}

// }}}
// {{{ sitespinner_sql_sync($source_alias, $destination_alias)

function sitespinner_sql_sync($source_alias, $destination_alias) {
//    drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sync.sql');
//    drush_sql_sync($source_alias, $destination_alias);
  drush_invoke_process($source_alias, 'sql-sync', $commandline_args = array(
    $source_alias,
    $destination_alias,
  ), $commandline_options = array(
    'yes' => TRUE,
    'no-cache' => TRUE,
  ), $backend_options = TRUE);
}

/**
 * Entrypoint for drush rsync.
 *
 * @param source
 *   A site alias ("@dev") or site specification ("/path/to/drupal#mysite.com")
 *   followed by an optional path (":path/to/sync"), or any path
 *   that could be passed to rsync ("user@server.com:/path/to/dir/").
 * @param site
 *   The site name being created.
 */
function sitespinner_rsync_files($base, $destination) {

  sitespinner_print("Copying files directory: \n");

  $destination_alias_record = sitespinner_drush_sitealias_get_record($destination);
  $multisite_root = $destination_alias_record['root'] . '/sites/' . str_replace('/', '.', $destination_alias_record['uri']);
  $destination_files_directory = !empty($destination_alias_record['sitespinner-destination']['path-aliases']['%files']) ?
    $destination_alias_record['sitespinner-destination']['path-aliases']['%files'] :
    $multisite_root . '/files';
  $default_user = !empty($destination_alias_record['sitespinner-destination']['server-environment']['default_user']) ?
    $destination_alias_record['sitespinner-destination']['server-environment']['default_user'] :
    SITESPINNER_DEFAULT_USER;
  $default_group = !empty($destination_alias_record['sitespinner-destination']['server-environment']['default_group']) ?
    $destination_alias_record['sitespinner-destination']['server-environment']['default_group'] :
    SITESPINNER_DEFAULT_GROUP;
  $base .= ':%files';
  $destination .= ':%files';
  drush_invoke_process($base, 'rsync', $commandline_args = array(
    $base,
    $destination,
  ), $commandline_options = array(
    'yes' => TRUE,
    'exclude' => '*/settings.php'
  ), $backend_options = TRUE);

  sitespinner_print("Fixing file ownership and permissions\n");
  sitespinner_exec("sudo chown -R " . $default_user . ":" . $default_group . " " . $destination_files_directory);
  $chmod_command = dt("for x in @files; do
      find \${x} -type d -exec chmod ug=rwx,o= '{}' \\;
      find \${x} -type f -exec chmod ug=rw,o= '{}' \\;
    done",
    array('@files' => $destination_files_directory));
  sitespinner_print($chmod_command);
  sitespinner_exec($chmod_command);
}

// }}}
// {{{ sitespinner_enable_db($site)

/**
 * Create a database using the db specs in the $alias record.
 *
 * @param array $alias_record
 *  The site alias record for the destination database. Must include a database
 *  specification as an array in the form used in settings.php in the
 *  ['sitespinner-destination'] portion of the alias. E.g.
 *  $alias_record['databases']['default']['default'] = array(
 *    'database' => 'database_name',
 *    'username' => 'user_name',
 *    'password' => 'password',
 *    'host'     => 'localhost',
 *    'port'     => '',
 *    'driver'   => 'mysql',
 *  );
 *
 * @return array
 *    containing the newly created database info or 0 on failure
 */
function sitespinner_enable_db($alias_record) {

  sitespinner_print(dt("Proceeding and trying to create database. \nYou may be prompted for password: ") . "\n");

  // Check to see if this database already exists and if we can connect to it
  if (empty($alias_record['databases']['default']['default'])) {
    return drush_set_error(dt("Database configuration needs to exist in 'sitespinner-destination' section of the destination site alias"));
  }
  $db_spec = $alias_record['databases']['default']['default'];

  // Use db_creator credentials, if provided.
  if (!empty($alias_record['sitespinner-destination']['db_creator'])) {
    $db_spec['username'] = $alias_record['sitespinner-destination']['db_creator']['username'];
    $db_spec['password'] = $alias_record['sitespinner-destination']['db_creator']['password'];
  }

  if (!sitespinner_create_db($db_spec)) {
    return 0;
  }
  else {
    return $db_spec;
  }

}

// }}}
// misc helper functions
// {{{ sitespinner_write_settings($setttings_file, $settings)

/**
 * function for writing to a file and controlling how to exit on error
 *
 * @param
 *   string $settings_file
 * @param
 *   string $settings to write
 */
function sitespinner_write_settings($settings_file, $settings) {
  $force = drush_get_option('f');
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  if ($simulate) {
    sitespinner_print(SIMULATE_PREPEND . dt("Writing new settings.php file: " . $settings_file) . "\n");
    return 1;
  }

  $res = sitespinner_write_file($settings_file, $settings);
  if (!$res) {
    if (empty($silent)) {
      sitespinner_print(ERROR_PREPEND . dt("Could not write new settings.php: " . $settings_file) . "\n");
    }
    if (empty($force)) {
      exit(1);
    }
  }
  else {
    if (empty($silent)) {
      sitespinner_print(NO_ERROR_PREPEND . dt("Writing new settings.php file: " . $settings_file) . "\n");
    }
  }
}

// }}}
// {{{ sitespinner_get_file($filename)

/**
 * @param
 *  string $filename The filename to open and read
 * @return
 *  string $content of the file which has been read
 */
function sitespinner_get_file($filename) {
  $force = drush_get_option('f');
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  if ($simulate) {
    sitespinner_print(dt("Will try and read contents of $filename") . "\n");
    return 1;
  }

  clearstatcache();
  $content = fread($fp = fopen($filename, 'r'), max(1, filesize($filename)));
  fclose($fp);
  return $content;
}

// }}}
// {{{ sitespinner_write_file($filename, $contents)

/**
 * @param
 *  string $filename The filename to write
 * @param
 *  string $contents the content to write
 */
function sitespinner_write_file($filename, $contents) {
  if ($fp = @fopen($filename, 'w')) {
    flock($fp, 2);
    fwrite($fp, $contents);
    flock($fp, 3);
    fclose($fp);
    return 1;
  }
  else {
    return 0;
  }
}

// }}}
// {{{ sitespinner_readline($line)

/**
 * funtion for wrapping the readline function
 * @param string ouput to print to screen
 * @return string the input which readline reads
 */
function sitespinner_readline($line) {
  if (function_exists('readline')) {
    $ret = readline($line);
    return $ret;
  }
  else {
    // if not readline we use this
    // we could actually just use this .)
    // found at php.net
    print $line;
    $out = "";
    $key = "";
    $key = fgetc(STDIN);        //read from standard input (keyboard)
    while ($key != "\n") {       //if the newline character has not yet arrived read another
      $out .= $key;
      $key = fread(STDIN, 1);
    }
    return $out;
  }
}

// }}}

function sitespinner_update_etc_hosts($hostname) {
  // create new hosts file and reload server
  $hosts_file_str = file_get_contents("/etc/hosts");
  $new_host = "127.0.0.1\t$hostname\n";
  if (!strstr($hosts_file_str, $new_host)) {
    $new_hosts_file_str = $new_host . $hosts_file_str;
    file_put_contents("/tmp/hosts", $new_hosts_file_str);
    sitespinner_exec("sudo cp -f /tmp/hosts /etc/hosts");
    sitespinner_exec("sudo /etc/init.d/apache2 reload");
    sitespinner_exec("rm /tmp/hosts");
  }
}

/**
 * Get a site alias record with sitespinner data recursively merged from parents
 *
 * @param $alias_name
 * @return array
 */
function sitespinner_drush_sitealias_get_record($alias_name) {

 $site_alias_record = &sitespinner_static(__FUNCTION__ . $alias_name);
  if (!isset($site_alias_record)) {

    $raw_site_alias_record = _drush_sitealias_load_alias($alias_name);
    $site_alias_record = drush_sitealias_get_record($alias_name);
    if (!empty($raw_site_alias_record['parent'])) {
      // Fetch and merge in each parent's sitespinner configuration
      foreach (explode(',', $raw_site_alias_record['parent']) as $parent) {
        $parent_record = sitespinner_drush_sitealias_get_record($parent);

        if (!empty($parent_record['sitespinner-destination'])) {
          if (empty($raw_site_alias_record['sitespinner-destination'])) {
            $site_alias_record['sitespinner-destination'] = $parent['sitespinner-destination'];
          }
          else {
            $site_alias_record['sitespinner-destination'] = _sitespinner_array_merge_recursive_distinct($parent_record['sitespinner-destination'], $raw_site_alias_record['sitespinner-destination']);
          }
        }
        $site_alias_record = array_merge($parent_record, $site_alias_record);
      }
    }
  }

  return $site_alias_record;
}


/**
 * Recursively merge two arrays while keeping scalar values unique
 *
 * PHP's native array_merge_recursive function, when the two arrays having a scalar
 * value at a given array index, converts that value to an array with the values
 * from both source arrays. This version uses the value from the second array.
 *
 * @param array $array1
 * @param array $array2
 * @return array
 */
function _sitespinner_array_merge_recursive_distinct ( array &$array1, array &$array2 )
{
  $merged = $array1;

  foreach ( $array2 as $key => &$value )
  {
    if ( is_array ( $value ) && isset ( $merged [$key] ) && is_array ( $merged [$key] ) )
    {
      $merged [$key] = _sitespinner_array_merge_recursive_distinct ( $merged [$key], $value );
    }
    else
    {
      $merged [$key] = $value;
    }
  }

  return $merged;
}

/**
 * Vset values from the 'variables' array in an alias file to the site
 *
 * The variables array consists of name => data pairs, and is found at
 * ['sitespinner-destination']['variables'] in the site alias array.
 * Array data values are automatically serialized by vset, and should not be
 * serialized ahead of time.
 *
 * @param string $destination
 *   a
 */
function sitespinner_set_variables($destination) {
  $destination_site_alias_record = sitespinner_drush_sitealias_get_record($destination);
  if (!empty($destination_site_alias_record['sitespinner-destination']['variables'])) {
    foreach($destination_site_alias_record['sitespinner-destination']['variables'] as $name => $data) {
      sitespinner_set_variable($destination, $name, $data);
    }
  }
}

/**
 * Set a variable on a site referenced by an alias
 *
 * @param string $alias
 * @param string $name
 * @param mixed $data
 */
function sitespinner_set_variable($alias, $name, $data) {
  $commandline_options = array();
  if (!is_string($data)) {
    $data = json_encode($data);
    $commandline_options['format'] = 'json';
  }
  drush_invoke_process($alias, 'vset', $commandline_args = array(
    $name,
    $data,
  ), $commandline_options, $backend_options = TRUE);
}

/**
 * Clone of drupal_static function
 *
 * @param $name
 * @param null $default_value
 * @param bool $reset
 * @return mixed
 */
function &sitespinner_static($name, $default_value = NULL, $reset = FALSE) {
  static $data = array(), $default = array();
  // First check if dealing with a previously defined static variable.
  if (isset($data [$name]) || array_key_exists($name, $data)) {
    // Non-NULL $name and both $data[$name] and $default[$name] statics exist.
    if ($reset) {
      // Reset pre-existing static variable to its default value.
      $data [$name] = $default [$name];
    }
    return $data [$name];
  }
  // Neither $data[$name] nor $default[$name] static variables exist.
  if (isset($name)) {
    if ($reset) {
      // Reset was called before a default is set and yet a variable must be
      // returned.
      return $data;
    }
    // First call with new non-NULL $name. Initialize a new static variable.
    $default [$name] = $data [$name] = $default_value;
    return $data [$name];
  }
  // Reset all: ($name == NULL). This needs to be done one at a time so that
  // references returned by earlier invocations of drupal_static() also get
  // reset.
  foreach ($default as $name => $value) {
    $data [$name] = $value;
  }
  // As the function returns a reference, the return should always be a
  // variable.
  return $data;
}