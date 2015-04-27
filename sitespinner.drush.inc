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
 * the default user of all the web sites
 * this will be the owner of the settings.php file created by the sitespinner script
 * We set this to the user who is running the drush command
 */

define ('SITESPINNER_DEFAULT_USER', getenv('USER'));

/**
 * this will be the group of the settings.php file created by the sitespinner script.
 * On my system the default apache user is www-data (ubuntu). If we set the files
 * directory to be owned by this group we insure that only apache can read and
 * write the files. Also if we set the group of the settings file to this group,
 * we insure that the server can read the settings file
 */
define ('SITESPINNER_DEFAULT_GROUP', '_www');

/**
 * Permissions for the settings.php file
 * if we set the settings.php file to 740 we insure that only the SITESPINNER_DEFAULT_USER
 * and SITESPINNER_DEFAULT_GROUP can read the settings.php file. User get full rights.
 */
define ('SITESPINNER_SETTINGS_PERMS', '740');

/**
 * Permissions for the files/ folder
 * if we set the files/ folder to 770 we insure that only the SITESPINNER_DEFAULT_USER
 * and SITESPINNER_DEFAULT_GROUP can read and write to this folder
 */
define ('SITESPINNER_FILES_DIR_PERMS', '770');


// {{{ sitespinner_drush_command()

/**
 * Implementation of hook_drush_command().
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return array
 *   An associative array describing your command(s).
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
  );

  return $items;
}

// }}}
// {{{ sitespinner_drush_help()

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function sitespinner_drush_help($section) {
  switch ($section) {
    case 'drush:sitespinner':
      return dt("drush sitespinner is a script that creates a new multi-site cloned from an existing base site. In detail, it: \n* Creates a new folder inside the /sites/ folder for the new site\n* Copies over the base site's :files directory.\n* Creates a new database and copies the data from the base site's database.\n* Creates a settings.php file.\n* Root and site-specific configuration overrides may be provided in the destination site alias.
\nThe command requires two drush site aliases as arguments: base site, and destination site. Therefore, before you can spin up a new site, you need to have created site aliases for both of these.
\nConsult the sample aliases file included with this command for complete instructions.");
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

  $base_site_alias_record = drush_sitealias_get_record($base);
  if (!$base_site_alias_record) {
    sitespinner_print(dt("'@alias' was not recognized as a valid source site alias. Exit. ", array('@alias' => $base)) . "\n");
    return 1;
  }
  $destination_site_alias_record = sitespinner_drush_sitealias_get_record($destination);
  if (!$destination_site_alias_record) {
    sitespinner_print(dt("'@alias' was not recognized as a valid destination site alias. Exit. ", array('@alias' => $destination)) . "\n");
    return 1;
  }

  $drupal_root = $destination_site_alias_record['root'];
  $parse_uri = explode('/', $destination_site_alias_record['uri']);
  $site = end($parse_uri);

//  print_r($base_site_alias_record);
//  print_r($destination_site_alias_record);

  if (empty($destination_site_alias_record['sitespinner-destination']['create-domain']['type']) || !(in_array($destination_site_alias_record['sitespinner-destination']['create-domain']['type'], array('path', 'domain', 'subdomain')))) {
    $types = array('path', 'domain', 'subdomain');
    $reply = drush_choice($types, 'Please indicate the domain type to use for this site by entering a number from the following list.', '!value');
    if ($reply === FALSE) {
      return 1;
    }
    $domain = $types[$reply];
    sitespinner_print(dt("\"@domain\" was chosen for the domain setting.\n", array('@domain' => $domain)));
  }

  // sudo :         sudo to ensure that we can log in as su
  // database:      enable a database or use existing
  // drupal:        create files for site/ and site/files
  // apache:        create apache log files and configuration and restart apache2
  // load database: ask to populate the database
  // cron:          enable cron job

  $res = sitespinner_exec("sudo echo -e " . dt("Started enabling a sub-site"));
  if ($res) {
    sitespinner_print(dt("Could not sudo. Exit"));
    return 1;
  }

  $destination_db_spec = sitespinner_enable_db($destination_site_alias_record);
  if (!$destination_db_spec) {
    sitespinner_print(dt("Could not make database. Exit\n"));
    return 1;
  }

  print_r($destination_db_spec);
  sitespinner_create_site($base_site_alias_record, $destination_site_alias_record);

  // Copy database and files from source.
  sitespinner_sql_sync($base, $destination);
  sitespinner_rsync_files($base, $destination);

  // Handle domain setting.
  switch($domain) {
    case 'domain':
      // TODO vhost stuff.
      //  sitespinner_create_virtualhost($site);
    break;
    case 'subdomain':
      // TODO vhost stuff.
      //  sitespinner_create_virtualhost($site);
      break;
    default:
      // Create a to-self symlink using multisite name in the drupal root
      sitespinner_exec(dt('sudo ln -s . @uri ', array('@uri' => $drupal_root . "/" . $site)));
      break;
  }


  // check to see if user wants a default cron line
//  sitespinner_enable_cron($site);
//  sitespinner_print_status($site, $db_res);
  return 0;
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
 * function for creating new drupal site folder in e.g.:
 * sites/example.org
 *
 * @param
 *   string $site the drupal website to enable
 * @param
 *   array $db_spec an array holding the db_spec for rewriting the $db_url in settings.php
 * @return
 *   void all error handling is being done in sitespinner_exec
 */

// FIX: Maybe check if directory is writable and return 1 or 0.
function sitespinner_create_site($base_site_alias_record, $destination_site_alias_record) {

  // create settings dir 
  // cp settings file
  sitespinner_exec("mkdir " . $destination_site_alias_record['root']);
  $destination_settings_file_path = $destination_site_alias_record['root'] . "/settings.php";
  $drupal_root = $destination_site_alias_record['root'];
  if (!empty($destination_site_alias_record['settings_file_template'])) {
    $source_settings_file_path = $destination_site_alias_record['settings_file_template'];
    if(strpos('/', $source_settings_file_path) !== 0) {
      $source_settings_file_path = $drupal_root . "/" . $source_settings_file_path;
    }
  }
  else {
    $source_settings_file_path = $drupal_root . "/sites/default/default.settings.php";
  }
  sitespinner_exec("cp " . $source_settings_file_path . " " . $destination_settings_file_path);

  // search and replace $db_url and write new settings file
  $settings_string = sitespinner_get_file($destination_settings_file_path);

  // search and replace $db_url and write new settings file
  $database_settings_string = var_export($destination_site_alias_record['sitespinner-destination']['databases'], TRUE);
  $search = "\n\$databases = array();\n";
  $replace = "\n\$databases = " . $database_settings_string . ";\n";
  $settings_string = str_replace($search, $replace, $settings_string);
  sitespinner_write_settings($destination_settings_file_path, $settings_string);

  $destination_files_directory = !empty($destination_site_alias_record['sitespinner-destination']['path-aliases']['%files']) ?
    $destination_site_alias_record['sitespinner-destination']['path-aliases']['%files'] :
    $destination_site_alias_record['root'] . '/files';
  // secure settings for files and settings
  sitespinner_exec("sudo mkdir " . $destination_files_directory);
  sitespinner_exec("sudo chmod " . SITESPINNER_FILES_DIR_PERMS . " " . $destination_files_directory);
  sitespinner_exec("sudo chown " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $destination_files_directory);
  sitespinner_exec("sudo chmod " . SITESPINNER_SETTINGS_PERMS . " " . $destination_settings_file_path);
  sitespinner_exec("sudo chown " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $destination_settings_file_path);
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
  if ($db_spec['driver'] == 'mysql' || $db_spec['driver'] == 'mysqli') {
    $command = "mysqladmin -h" . $db_spec['host'] . " ";
    $command .= "-p" . $db_spec['password'] . " ";
    $command .= "-u" . $db_spec['username'] . " ";
    $command .= "create " . $db_spec['database'];
    sitespinner_exec($command);
  }
  else {
    if ($db_spec['driver'] == 'pgsql') {
      $command = "export PGPASSWORD='$db_spec[password]'; ";
      $command .= "createdb -O " . $db_spec['username'] . " ";
      $command .= "-U " . $db_spec['username'] . " ";
      $command .= "-h " . $db_spec['host'] . " ";
      if (!empty($db_spec['port'])) {
        $command .= "-p " . $db_spec['port'] . " ";
      }
      $command .= "-E " . SITESPINNER_SQL_ENCODING . " " . $db_spec['database'];
      sitespinner_exec($command);
    }
    else {
      sitespinner_print(dt("Unknown driver: ") . $db_spec['driver'] . "\n");
      return 0;
    }
  }
  return 1;
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
  if ($db_spec['driver'] == 'mysql' || $db_spec['driver'] == 'mysqli') {
    $command = "mysqladmin --force ";
    $command .= "-h" . $db_spec['host'] . " ";
    $command .= "-p" . $db_spec['password'] . " ";
    $command .= "-u" . $db_spec['username'] . " drop ";
    $command .= $db_spec['database'];
    sitespinner_exec($command);
  }
  else {
    if ($db_spec['driver'] == 'pgsql') {
      $command = "export PGPASSWORD='$db_spec[password]';";
      $command .= "dropdb ";
      $command .= "-h " . $db_spec['host'] . " ";
      $command .= "-U " . $db_spec['username'] . " ";
      if (!empty($db_spec['port'])) {
        $command .= "-p " . $db_spec['port'] . " ";
      }
      $command .= '"' . $db_spec['database'] . '"';
      sitespinner_exec($command);
    }
    else {
      sitespinner_print(dt("Unknown driver: ") . $db_spec['driver']);
      return 0;
    }
  }
  return 1;
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
  $destination_files_directory = $destination_alias_record['path-aliases']['%files'];
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
  sitespinner_exec("sudo chown -R " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $destination_files_directory);
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
 *  $alias_record['sitespinner-destination']['databases']['default']['default'] = array(
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
  if (empty($alias_record['sitespinner-destination']['databases']['default']['default'])) {
    return drush_set_error(dt("Database configuration needs to exist in 'sitespinner-destination' section of the destination site alias"));
  }
  $db_spec = $alias_record['sitespinner-destination']['databases']['default']['default'];

  // We only know how to handle mysql - sorry!
  // TODO support other database types
  if (!empty($db_spec['driver']) && $db_spec['driver'] != 'mysql') {
    return drush_set_error(dt("'!driver' databases are not supported at this time. Exit.", array('!driver' => $db_spec['driver'])));
  }

  // Use db_creator credentials, if provided.
  if (!empty($alias_record['sitespinner-destination']['db_creator'])) {
    $db_spec['username'] = $alias_record['sitespinner-destination']['db_creator']['username'];
    $db_spec['password'] = $alias_record['sitespinner-destination']['db_creator']['password'];
  }

  if (sitespinner_database_exists($db_spec)) {
    sitespinner_print("\n" . dt("Database ") . $db_spec['database'] . dt(" already exists and user can connect to it.") . "\n");
    $readline_str = dt("Do you want to drop the given database and create it again") . dt(" [Y/n]? ");
    $db_use_exists = strtoupper(sitespinner_readline($readline_str));

    // Check to see if the user wants to use existing database
    if ($db_use_exists == 'Y' || empty ($db_use_exists)) {

      // delete database
      if (!sitespinner_delete_db($db_spec)) {
        sitespinner_print(dt("Could not drop database. ") . $db_spec['database'] . "\n");
        return 0;
      }
      // continue and create databse
      if (!sitespinner_create_db($db_spec)) {
        return 0;
      }
      else {
        return $db_spec;
      }
    }
    // database does not exist. Lets create it.
  }
  else {
    if (!sitespinner_create_db($db_spec)) {
      sitespinner_enable_db($alias_record, 1);
      return 0;
    }
    else {
      return $db_spec;
    }
  }
  return 0;
}

// }}}
// {{{ sitespinner_database_exists($db_spec)
/**
 * function for checking if database exists.
 * @param
 *   array $db_spec an array with the information about the database
 * return
 *   int  1 if exists 0 if not
 */
function sitespinner_database_exists($db_spec) {
  // the command returns with positive int if mysql can not use 
  // the database specified in $db_spec[database]
  if ($db_spec['driver'] == 'mysql' || $db_spec['driver'] == 'mysqli') {
    $res = sitespinner_exec("echo 'use $db_spec[database]' | mysql -u$db_spec[username] -p$db_spec[password] -h$db_spec[host]", 1);
    if ($res) {
      return 0;
    }
    else {
      return 1;
    }
  }
  else {
    if ($db_spec['driver'] == 'pgsql') {
      // this shell command returns 1 if a pgsql database is found with the name $db_spec['database'];
      $command = "export PGPASSWORD='$db_spec[password]';";
      $command .= "sql=\"select count(1) from pg_catalog.pg_database where datname = '$db_spec[database]'\" && ";
      $command .= "cmd=\"psql -U$db_spec[username] -h$db_spec[host] -t -c \\\"\$sql\\\"\"";
      $command .= " && db_exists=`eval \$cmd` && exit \$db_exists";

      // FIX: NICER WAYS TO DO THIS
      // print $command . "\n";
      $res = sitespinner_exec($command, 1);
      // print "res $res";
      if ($res) {
        return 1;
      }
      else {
        return 0;
      }
    }
  }
}

// }}}
// misc helper functions
// {{{ sitespinner_get_settings_filename ($site)

/**
 * function for getting settings filenames when enabling a sub-site
 * @param   string $site
 * @return  string  $settings_filename
 */
function sitespinner_get_settings_filename($site) {
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $settings_filename = $drupal_root . "/sites/" . $site . "/settings.php";
  return $settings_filename;
}

// }}}
// {{{ sitespinner_get_sites_root($path)

/**
 *  function for finding base_path of all sites
 * @param
 *    string $path the path to search in
 * @return
 *    string $base the base_path of all sites
 */
function sitespinner_get_sites_root($path) {
  $str = SITESPINNER_WWW_HOME;
  if (!empty($str)) {
    return $str;
  }
  // we remove the /htdocs level: we have e.g. 
  // /home/user/www/www.www.os-cms.net
  // we remove the base site level: we have e.g. 
  // /home/user/www 
  $path = explode('/', $path);
  array_pop($path);
  array_pop($path);
  $base = implode('/', $path);
  return $base;
}

// }}}
// {{{ sitespinner_get_filenames($site)

/**
 * function for creating an array of the most used names when configuring
 * the drupal site and the apache server
 *
 * @param
 *   string   $site
 * @return
 *   array    $filenames  an array of filenames
 */
function sitespinner_get_filenames($site) {
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');

  $settings_file = sitespinner_get_settings_filename($site);
  $www_base = sitespinner_get_sites_root($drupal_root);

  // find the default apache2.conf file
  $path = explode('/', __FILE__);
  array_pop($path);
  $apache_default_conf = implode('/', $path) . "/apache2/" . SITESPINNER_APACHE_FILE;

  $major_version = substr(drush_drupal_version(), 0, 1);
  $settings_file_default = implode('/', $path) . "/settings/default.settings.php-" . $major_version . ".x";

  $filenames = array(
    'drupal_root' => $drupal_root,
    'drush_command' => $drush_command,
    'settings_file' => $settings_file,
    'settings_dir' => $drupal_root . "/sites/" . $site,
    'settings_file_default' => $settings_file_default,
    'files_dir' => $drupal_root . '/sites/' . $site . '/files',
    'www_base' => $www_base,
    'www_base_site' => $www_base . "/" . $site,
    'www_base_htdocs' => $www_base . "/" . $site . "/" . SITESPINNER_APACHE_HTDOCS,
    'www_base_site_logs' => $www_base . "/" . $site . "/" . SITESPINNER_APACHE_LOGS,
    'www_base_site_access' => $www_base . "/" . $site . "/" . SITESPINNER_APACHE_LOGS . "/access.log",
    'www_base_site_error' => $www_base . "/" . $site . "/" . SITESPINNER_APACHE_LOGS . "/error.log",
    'apache_default_conf' => $apache_default_conf,
    'apache_sites_avail' => SITESPINNER_APACHE_SITES_AVAIL . "/" . $site,
  );
  return $filenames;
}

// }}}
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

  $raw_destination_site_alias_record = _drush_sitealias_load_alias($alias_name);
  $destination_site_alias_record = drush_sitealias_get_record($alias_name);
  if (!empty($raw_destination_site_alias_record['parent'])) {
    // Fetch and merge in each parent's sitespinner configuration
    foreach (explode(',', $raw_destination_site_alias_record['parent']) as $parent) {
      $parent_record = sitespinner_drush_sitealias_get_record($parent);
      if(!empty($parent_record['sitespinner-destination'])) {
        if(empty($raw_destination_site_alias_record['sitespinner-destination'])) {
          $destination_site_alias_record['sitespinner-destination'] = $parent['sitespinner-destination'];
        }
        else {
          $destination_site_alias_record['sitespinner-destination'] = _sitespinner_array_merge_recursive_distinct($parent_record['sitespinner-destination'], $raw_destination_site_alias_record['sitespinner-destination']);
        }
      }
      $destination_site_alias_record = array_merge($parent_record, $destination_site_alias_record);
    }
  }

  return $destination_site_alias_record;
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