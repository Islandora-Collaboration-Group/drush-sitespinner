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

// {{{ prepend.
// include configuration file.
$sitespinner_conf = dirname(__FILE__) . "/sitespinner.conf";
if (file_exists($sitespinner_conf)) {
  require_once $sitespinner_conf;
}
else {
  $sitespinner_conf_dist = dirname(__FILE__) . "/dist-sitespinner.conf";
  require_once $sitespinner_conf_dist;
}
// }}}

// {{{ sitespinner_init()
/**
 * Function for checking current configuration.
 *
 * @return int
 *   1 = success
 *   0 = fail
 */
function sitespinner_init() {
  $sitespinner_dir = dirname(__FILE__);
  $sitespinner_conf = dirname(__FILE__) . "/sitespinner.conf";
  $sitespinner_conf_dist = dirname(__FILE__) . "/dist-sitespinner.conf";

//  $sitespinner_apache2_dir = dirname(__FILE__);
//  $sitespinner_apache2_conf = dirname(__FILE__) . "/apache2/apache2.conf";
//  $sitespinner_apache2_conf_dist = dirname(__FILE__) . "/apache2/dist-apache2.conf";

  $sitespinner_sql_dir = dirname(__FILE__) . "/sql";

  if (file_exists($sitespinner_conf) /*&& file_exists($sitespinner_apache2_conf)*/ && file_exists($sitespinner_sql_dir)) {
    return 1;
  }

  if (!file_exists($sitespinner_conf)) {
    $output = dt("Configuration file not found: ") . $sitespinner_conf . "\n";
    sitespinner_print($output);
    $readline_str = dt("Do you want to create: ") . $sitespinner_conf;
    $readline_str .= dt(" from ") . $sitespinner_conf_dist . dt(" [Y/n]?");
    $create_sitespinner_conf = strtoupper(sitespinner_readline($readline_str));
    if ($create_sitespinner_conf == 'Y' || empty($create_sitespinner_conf)) {
      sitespinner_exec("cp " . $sitespinner_conf_dist . " " . $sitespinner_conf);
    }
    else {
      sitespinner_print(dt("We can not proceed without a default configuration file. Exit" . "\n"));
      return 0;
    }
  }

//  if (!file_exists($sitespinner_apache2_conf)) {
//    $output = dt("Apache2 configuration file not found: ") . $sitespinner_apache2_conf . "\n";
//    sitespinner_print($output);
//    $readline_str = dt("Do you want to create: ") . $sitespinner_apache2_conf;
//    $readline_str .= dt(" from ") . $sitespinner_apache2_conf_dist . dt(" [Y/n]?");
//    $create_apache2_conf = strtoupper(sitespinner_readline($readline_str));
//    if ($create_apache2_conf == 'Y' || empty($create_apache2_conf)) {
//      sitespinner_exec("cp " . $sitespinner_apache2_conf_dist . " " . $sitespinner_apache2_conf);
//    }
//    else {
//      sitespinner_print(dt("We can not proceed without a default apache file. Exit" . "\n"));
//      return 0;
//    }
//  }

  if (!file_exists($sitespinner_sql_dir)) {
    $output = dt("Sql dir not found: ") . $sitespinner_sql_dir . "\n";
    sitespinner_print($output);
    $readline_str = dt("Do you want to create: ") . $sitespinner_sql_dir . dt(" [Y/n]?");
    $create_sql_dir = strtoupper(sitespinner_readline($readline_str));
    if ($create_sql_dir == 'Y' || empty($create_sql_dir)) {
      sitespinner_exec("mkdir " . $sitespinner_sql_dir);
    }
    else {
      sitespinner_print(dt("We can not proceed without a sql dir. Exit" . "\n"));
      return 0;
    }
  }
  sitespinner_print(dt("Look at the two configuration files created and change them as you like:") . "\n");
  sitespinner_print($sitespinner_conf . "\n");
//  sitespinner_print($sitespinner_apache2_conf . "\n");
  return 1;
}

// }}}
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

  $site_options = array(
    'source-alias' => dt('A drush site alias name to define the base site that will be cloned to create the multi-site. E.g. "@base-site".  (To use this option, this site alias file must have been created and installed in your drush configuration.)'),
    'domain' => dt('["path"], "subdomain", "domain": If "path" (the default) is entered, the new site will be found at !domain/yoursitename. If "domain" is entered, a new domain will be created at yoursitename. If "subdomain" is entered, a new subdomain will be created at yoursitename.!domain', array('!domain' => 'your.domain')),
    'site-config' => dt('Path to a file with configuration options for the new site. These will override settings from the base site.'),
    'root-config' => dt('Path to a file with configuration options for the new site. These will override settings from the base site.'),
  );

  // The key in the $items array is the name of the command.
  $items['sitespinner'] = array(
    // The name of the function implementing your command.
    'callback' => 'sitespinner_sitespinner',
    // A short description of your command.
    'description' => dt('Enable a drupal multi-site install. Will create the specified multi-site in the base /sites folder. E.g. yourmultisite will be placed in sites/www.base-site.net.yourmultisite'),
    'arguments' => array(
      'site' => dt('The drupal sub-site to create. Use the "domain" option if this site should be reachable on its own domain or subdomain.'),
    ),
    'options' => array_merge($site_options, $options),
  );

  $items['dissite'] = array(
    // the name of the function implementing your command.
    'callback' => 'sitespinner_dissite',
    // a short description of your command
    'description' => dt("Disables a drupal sub-site. Will remove the spcified sub-site. e.g. www.yoursite.net in sites/www.yoursite.net will be removed."),
    'arguments' => array(
      'site' => dt('The drupal site to disable. e.g. yoursite.net. Only works when a drupal install is enabled.'),
    ),
    'options' => $options,
  );

  $items['listsites'] = array(
    // the name of the function implementing your command.
    'callback' => 'sitespinner_list_all_sites',
    // a short description of your command
    'description' => dt("Lists all drupal base sites and all sub-sites of base sites enabled on your server."),
  );

  $items['sitespinner_test'] = array(
    // the name of the function implementing your command.
    'callback' => 'sitespinner_test',
    'description' => dt("Used for testing"),
    // a short description of your command
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
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
    case 'drush:dissite':
      return dt("dissite disables a site by removing the folder inside sites/ e.g. sites/www.yoursite.net and also the database and the virtual host.");
    case 'drush:sitespinner':
      return dt("drush sitespinner is a script that creates a new multi-site cloned from an existing base site. In detail, it: \n* Creates a new folder inside the /sites/ folder for the new site\n* Copies over the base site's :files directory.\n* Creates a new database and copies the data from the base site's database.\n* Creates a settings.php file.\n* Root and site-specific configuration overrides may be provided by additional configuration files.
\nThe base site is determined either by the current drush site context, or by including a base site drush alias as a command option.
To use, you must first cd to the sites/default or sites/<base sitename> folder, or include the --source-alias option in the command.
If 'domain' or 'subdomain' is specified in the command options, the command will create a <VirutalHost> configuration and enable it.");
    case 'drush:listsites':
      return dt("Lists all drupal base sites and all sub-sites to base sites enabled on your server.");
  }
}

// }}} sitespinner_drush_help ()
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
// {{{ sitespinner_sitespinner($site)

/**
 * function which will try to enable a sub-site to a drupal base site
 *
 * @param
 *    string $site the name of the site to enable
 * @return
 *    int 1 on failure on 0 on success. This will send exit code to
 *    the shell.
 */
function sitespinner_sitespinner($site) {

  $res = sitespinner_init();
  if (!$res) {
    return $res;
  }

  $force = drush_get_option('f');
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  $root_config = drush_get_option('root-config');
  $site_config = drush_get_option('site-config');
  $source_alias = drush_get_option('source-alias');
  $domain = drush_get_option('domain');

  $context_status = _core_site_status_table();


  // Make sure we are running inside a installed version of drupal.
  if (!$drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT')) {
    return dt("Could not find a valid install of drupal") . "\n";
  }

  if (empty($site)) {
    sitespinner_print(dt("You need to specify a site to create. "));
    $site = sitespinner_readline(dt("Enter sub-site name and hit return: "));
  }

  if (empty($domain) || !(in_array($domain, array('path', 'domain', 'subdomain')))) {
    sitespinner_print(dt("Please indicate the domain type to use for this site by entering a number from the following list\n"));
    $reply = 'none yet';
    while (!in_array($reply, array(1, 2, 3))) {
      if ($reply != 'none yet') {
        sitespinner_print(dt("\n\":reply\" was not a valid choice. Please indicate the domain type to use for this site by entering a number from the following list\n", array(':reply' => $reply)));
      }
      sitespinner_print(dt("[1] = \"path\" (default): the new site will be found at !domain/yoursitename\n", array('!domain' => 'your.domain')));
      sitespinner_print(dt("[2] = \"domain\": the new site will be found at yoursitename\n"));
      sitespinner_print(dt("[3] = \"subdomain\": the new site will be found at yoursitename.!domain\n", array('!domain' => 'your.domain')));
      $reply = sitespinner_readline(dt("Enter sub-site name and hit return: "));
      if ($reply == '') {
        $reply = 1;
      }
    }
    $choices = array(1 => 'path', 2 => 'domain', 3 => 'subdomain');
    $domain = $choices[$reply];
    sitespinner_print(dt("\"@domain\" was chosen for the domain setting.\n", array('@domain' => $domain)));

  }

  if (empty($root_config)) {
    sitespinner_print(dt("Do you want to use a root configuration file? "));
    $reply = sitespinner_readline(dt("y[n]: "));
    if (strtolower($reply) == 'y') {
      $root_config = sitespinner_readline(dt("Enter the path to the root configuration file and hit return: "));
    }
  }

  if (empty($site_config)) {
    sitespinner_print(dt("Do you want to use a site configuration file? "));
    $reply = sitespinner_readline(dt("y[n]: "));
    if (strtolower($reply) == 'y') {
      $site_config = sitespinner_readline(dt("Enter the path to the site configuration file and hit return: "));
    }
  }

  if (empty($site) && empty($force)) {
    sitespinner_print(dt("You need to specify a site. Exit. ") . "\n");
    return 1;
  }

  if (sitespinner_sub_exists($site) && empty ($force)) {
    sitespinner_print(dt("That site seems to exist as a sub-site already. Use the '-f' flag to force recreation. Exit.") . "\n");
    return 1;
  }

  if (sitespinner_base_exists($site) && empty ($force)) {
    sitespinner_print(dt("That site seems to exist as base site. Use the '-f' flag to force recreation. Exit.") . "\n");
    return 1;
  }

  // Load the root and site configurations.
  $cwd = drush_cwd();
  if ($root_config) {
    if (file_exists($cwd . '/' . $root_config)) {
      require_once $cwd . '/' . $root_config;
    }
    else {
      sitespinner_print(dt("Could not load root site configuration file. Exit.") . "\n");
      return 1;
    }
  }
  if ($site_config) {
    if (file_exists($cwd . '/' . $site_config)) {
      require_once $cwd . '/' . $site_config;
    }
    else {
      sitespinner_print(dt("Could not load site configuration file. Exit.") . "\n");
      return 1;
    }
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
  sitespinner_print("\n");
//  print_r(get_defined_vars());

  $destination_db_spec = sitespinner_enable_db($site);
  if (!$destination_db_spec) {
    sitespinner_print(dt("Could not make database. Exit\n"));
    return 1;
  }
  sitespinner_create_site($site, $destination_db_spec);
//  sitespinner_create_virtualhost($site);

  // Copy database and files from source.
  if ($source_alias) {
    sitespinner_sql_sync_from_alias($source_alias, $site);
    sitespinner_copy_files_from_source($source_alias, $site);
  }
  else {
    // Copy database and files from the current context.
    $source_db_spec = _drush_sql_get_db_spec();
    print_r($source_db_spec);
    if ($source_db_spec['driver'] == $destination_db_spec['driver'] && $destination_db_spec['driver'] == 'mysql') {
      $copy_db_command = sitespinner_create_mysql_copy_command($source_db_spec, $destination_db_spec);
      sitespinner_exec($copy_db_command);
    }
    else {
      sitespinner_print(dt('Unable to copy database. Source and destination must both use mysql driver. Exit'));
      return 1;
    }
    sitespinner_copy_files_from_source($context_status['root'] . '/' . $context_status['files'], $site);
  }

  // Handle domain setting
  switch($domain) {
    case 'domain':
      break;
    case 'subdomain':
      break;
    default:
      // create a symlink with the name $site in the /sites/ directory
      sitespinner_exec(dt('sudo ln -s . @root/@site ', array('@root' => $drupal_root, '@site' => $site)));
      break;
  }

//  // sitespinner_load_db
//  // keep trying to load the database until user rejects with a no,
//  // or when user has loaded a database
//  while (!$db_res = sitespinner_load_db($site, $destination_db_spec)) {
//  }

  // check to see if user wants a default cron line
//  sitespinner_enable_cron($site);
//  sitespinner_print_status($site, $db_res);
  return 0;
}

// }}}
// {{{ sitespinner_print_status
/*
 * function for printing status of install
 * @param   int  $db_res
 */
function sitespinner_print_status($site) {
  $status = dt("Manual install. Continue on http://") . $site . "/install.php\n";
  $status .= dt("Or use:\n\ncd $site/htdocs && drush site-install\n\n to do it from command line\n");
  sitespinner_print(dt($status));
  return 0;
}

// }}}
// {{{ sitespinner_dissite($site)

/**
 * function (callback) Disable a drupal sub-site
 * returns to the shell
 *
 * @param   string $site the drupal website to disable
 * @return  int 0 on success and 1 on failure
 */
function sitespinner_dissite($site) {
  $res = sitespinner_init();
  if (!$res) {
    return $res;
  }

  $force = drush_get_option('f');
  $silent = drush_get_option('s');
  $simulate = drush_get_option('simulate');

  $all_sites = array();
  $all_sites = sitespinner_get_sub_sites();

  if (empty($site)) {
    if (empty($all_sites) && empty($force)) {
      sitespinner_print(dt("There do not seem to be any valid sub-sites installed in this base.") . "\n");
      return 1;
    }
    else {
      sitespinner_print(dt("You need to specify a sub-site: Your choices are: \n"));
      sitespinner_list_sub_sites();
      $site = sitespinner_readline(dt("Enter site to disable: "));
    }
  }

  if (sitespinner_base_exists($site) && empty($force)) {
    sitespinner_print(dt("That site is a base site. Use the command 'drush dissitesa $site' to remove it.") . "\n");
    return 1;
  }


  if (!sitespinner_sub_exists($site) && empty($force)) {
    sitespinner_print(dt("That site does not seem to exist.") . "\n");
    return 1;
  }

  if (!sitespinner_sub_to_current($site)) {
    sitespinner_print(dt("That site is not a sub site to a current drupal base. You need to go to the correct base site.") . "\n");
    return 1;
  }

  sitespinner_exec('sudo echo -e "Started disabling a sub-site\n"');

  $settings = sitespinner_get_filenames($site);
  $readline_str = dt("Delete the following website: ") . $site . dt(" [Y/n]? ");
  $site_delete_confirm = strtoupper(sitespinner_readline($readline_str));
  if ($site_delete_confirm == 'Y' || empty($site_delete_confirm)) {

    // delete database
    $result = sitespinner_disable_db($settings['settings_file']);

    $readline_str = dt("Remove apache2 configuration and reload apache: ") . $site . dt(" [Y/n]? ");
    $apache2_delete_confirm = strtoupper(sitespinner_readline($readline_str));

    if ($apache2_delete_confirm == 'Y' || empty($apache2_delete_confirm)) {
      sitespinner_exec("sudo a2dissite " . $site);
      sitespinner_exec("sudo /etc/init.d/apache2 reload");
      sitespinner_exec("sudo rm " . $settings['apache_sites_avail']);
    }

    sitespinner_print(dt("Remove all site files found in: ") . "\n");
    sitespinner_print($settings['www_base_site'] . "\n" . $settings['settings_dir']);
    $readline_str = dt(" [Y/n]? ");
    $site_files_delete_confirm = strtoupper(sitespinner_readline($readline_str));

    if ($site_files_delete_confirm == 'Y' || empty($site_delete_confirm)) {
      sitespinner_exec("sudo rm -rf " . $settings['www_base_site']);
      sitespinner_exec("sudo rm -rf " . $settings['settings_dir']);
    }
    sitespinner_print(dt("Disabling site from crontab") . "\n");
    sitespinner_disable_cron($site);
  }

  return 0;
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
function sitespinner_create_site($site, $db_spec) {
  $settings = sitespinner_get_filenames($site);

  // create settings dir 
  // cp settings file
  sitespinner_exec("mkdir " . $settings['settings_dir']);
  sitespinner_exec("cp " . $settings['settings_file_default'] . " " . $settings['settings_file']);

  // search and replace $db_url and write new settings file
  $settings_string = sitespinner_get_file($settings['settings_file']);

  $search = "\n\$databases = array();\n";
  $replace = "\n\$databases = array (\n  'default' =>\n    array (\n      'default' => " . var_export($db_spec, TRUE) . ")\n);\n";
//  $replace = "\$db_url = " . "'$db_spec[driver]://$db_spec[username]:$db_spec[password]@$db_spec[host]/$db_spec[database]'";
  $settings_string = str_replace($search, $replace, $settings_string);
  print_r($replace);
  print_r($settings_string);

  sitespinner_write_settings($settings['settings_file'], $settings_string);

  // secure settings for files and settings
  sitespinner_exec("sudo mkdir " . $settings['files_dir']);
  sitespinner_exec("sudo chmod " . SITESPINNER_FOLDER_PERMS . " " . $settings['files_dir']);
  sitespinner_exec("sudo chown " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $settings['files_dir']);
  sitespinner_exec("sudo chmod " . SITESPINNER_SETTINGS_PERMS . " " . $settings['settings_file']);
  sitespinner_exec("sudo chown " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $settings['settings_file']);
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
// {{{ sitespinner_get_db_spec()

/**
 * Wrapper around the drush functions to get some db specs
 * At the time of writing these two functions returned different
 * stuff so i had to combine them.
 *
 * @return
 *   array  $db_spec array holding the information about the database
 *          of the base site where we are working.
 */
function sitespinner_get_db_spec() {
  $db_spec = _drush_sql_get_db_spec();
  // we need to use another version to get a correct driver 
  $db_spec_context = drush_get_context('DRUSH_DB_CREDENTIALS');
  $db_spec['driver'] = $db_spec_context['driver'];
  return $db_spec;
}

// }}}
// {{{ sitespinner_sync_source_db($source_alias)

function sitespinner_sql_sync_from_alias($source_alias, $site) {
  if (!empty($source_alias)) {
    // TODO set destination site as the context.
    $settings = sitespinner_get_filenames($site);
    sitespinner_exec("cd " . $settings['settings_dir']);
    drush_include(DRUSH_BASE_PATH . '/commands/sql', 'sync.sql');
    drush_sql_sync($source_alias, 'self');
  }
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
function sitespinner_copy_files_from_source($source, $site) {

  $dest_settings = sitespinner_get_filenames($site);
  sitespinner_print("Copying files directory: \n");

  if (strpos($source, '@') !== 0) {
    //
    $source .= '/';
    $exec = "rsync --exclude=*/settings.php " . $source . " " .  $dest_settings['files_dir'];
    sitespinner_exec($exec);
  }
  else {
    $source .= ':%files';
    drush_invoke_process($source, 'rsync', $commandline_args = array($source, $dest_settings['files_dir']), $commandline_options = array('yes' => TRUE, 'exclude' => '*/settings.php'), $backend_options = TRUE);

  }

  sitespinner_print("Fixing file ownership and permissions\n");
  sitespinner_exec("sudo chown -R " . SITESPINNER_DEFAULT_USER . ":" . SITESPINNER_DEFAULT_GROUP . " " . $dest_settings['files_dir']);
  $chmod_command = t("for x in @files; do
      find \${x} -type d -exec chmod ug=rwx,o= '{}' \\;
      find \${x} -type f -exec chmod ug=rw,o= '{}' \\;
    done",
    array('@files' => $dest_settings['files_dir']));
  sitespinner_print($chmod_command);
  sitespinner_exec($chmod_command);
}

// }}}
// {{{ sitespinner_enable_db($site)

/**
 * function for creating a database using readline commands
 * the function asks some questions and out of them the database
 * will be created
 *
 * @param
 *   string $site the name of the website to enable a database for
 * @param
 *   int  $install. If positive we are in install mode and 0 we are
 *        installaing a sub or base site.
 * @return
 *   array $db_spec containing the newly created database info
 *         or 0 on failure
 */
function sitespinner_enable_db($site, $install = 0) {
  $db_spec = array();
  $db_spec = sitespinner_get_db_spec();
  $db_name = preg_replace("[\.-]", "", $site);

  if ($install) {
    // if it's an install only use manual procedure
    $db_spec = sitespinner_enable_db_manual();
  }
  else {
    $output = dt("Found a default connection with:\n");
    $output .= dt("Driver:         ") . $db_spec['driver'] . "\n";
    $output .= dt("Host:           ") . $db_spec['host'] . "\n";
    $output .= dt("User:           ") . $db_spec['username'] . "\n";
    $output .= dt("Password:       ") . $db_spec['password'] . "\n\n";
    $output .= dt("Automatic generated name for database will be:") . "\n";
    $output .= dt("database        ") . $db_name . "\n\n";

    sitespinner_print($output);

    $readline_str = dt("Press 'Y' to use these settings. Press 'n' to proceed with manual install of database");
    $readline_str .= dt(" [Y/n]? ");
    $db_use_default = strtoupper(sitespinner_readline($readline_str));

    // proceeding manually
    if ($db_use_default == 'Y' || empty($db_use_default)) {
      $db_spec['database'] = $db_name;
    }
    else {
      $db_spec = sitespinner_enable_db_manual();
    }
  }

  sitespinner_print(dt("Proceeding and trying to create database. \nYou may be prompted for password: ") . "\n");

  // Check to see if this database already exists and if we can connect to it

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
      sitespinner_enable_db($site, $install);
      return 0;
    }
    else {
      return $db_spec;
    }
  }
  return 0;
}

// }}}
// {{{ sitespinner_enable_db_manual()

/**
 * function for enabling a database manually.
 * @return
 *   array $db_spec an array holding information about the database to be created
 */
function sitespinner_enable_db_manual() {
  $settings = array();
  sitespinner_print(dt("Manual install of the database. ") . "\n");
  if (!$db_spec['driver'] = SITESPINNER_DB_DRIVER) {
    sitespinner_print(dt("You PHP cli version reports you have support for these drivers: "));
    if (extension_loaded('mysql')) {
      sitespinner_print(dt('mysql '));
    }
    if (extension_loaded('mysqli')) {
      sitespinner_print(dt('mysqli '));
    }
    if (extension_loaded('pgsql')) {
      sitespinner_print(dt('pgsql '));
    }
    sitespinner_print("\n");
    $db_spec['driver'] = sitespinner_readLine(dt("Driver:         "));
  }
  else {
    sitespinner_print("Driver from settings from sitespinner.conf: " . $db_spec['driver'] . "\n");
  }
  if (!$db_spec['host'] = SITESPINNER_DB_HOST) {
    $db_spec['host'] = sitespinner_readLine(dt("Hostname:       "));
  }
  else {
    sitespinner_print("Host from sitespinner.conf: " . $db_spec['host'] . "\n");
  }
  if (!$db_spec['username'] = SITESPINNER_DB_USERNAME) {
    $db_spec['username'] = sitespinner_readLine(dt("Username:       "));
  }
  else {
    sitespinner_print("Usernames from sitespinner.conf: " . $db_spec['username'] . "\n");
  }
  if (!$db_spec['password'] = SITESPINNER_DB_PASSWORD) {
    $db_spec['password'] = sitespinner_readLine(dt("Password:       "));
  }
  else {
    sitespinner_print("Password from sitespinner.conf: " . $db_spec['password'] . "\n");
  }
  $db_spec['database'] = sitespinner_readLine(dt("Database name:  "));
  return $db_spec;
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
// {{{ sitespinner_disable_db()

/**
 * function for showing a confirm form on deleting a database
 *
 * @param
 *   string $settings_file the settings.php where the database will be deleted
 * @return
 *   int 0 on success postive int on failure
 */
function sitespinner_disable_db($settings_file = NULL) {

  $db_spec = _drush_sql_get_spec_from_settings($settings_file, 'source');
  $readline_str = dt("Delete the following database: ") . $db_spec['database'] . dt(" [Y/n]? ");
  $db_use_sitename = sitespinner_readline($readline_str);
  if ($db_use_sitename == 'Y' || empty($db_use_sitename)) {
    $result = sitespinner_delete_db($db_spec);
    return $result;
  }
}

// }}}
// {{{ sitespinner_save_dump($db_spec)

/**
 * @param
 *   array  $db_spec array holding information of the database to dump
 * @param
 *   string $dump_file, the string containing the path to the sql dump to be saved
 * @return
 *   int on success and 0 on failure
 */
function sitespinner_save_dump($db_spec, $dump_file) {
  // build command
  // execute it

  if ($db_spec['driver'] == 'mysql' || $db_spec['driver'] == 'mysqli') {
    $command = "mysqldump -u " . $db_spec['username'] . " ";
    $command .= "-p" . $db_spec['password'] . " ";
    $command .= "-h" . $db_spec['host'] . " ";
    $command .= $db_spec['database'] . " > " . $dump_file;
  }
  else {
    if ($db_spec['driver'] == 'pgsql') {
      $command = "export PGPASSWORD='$db_spec[password]';";
      $command .= "pg_dump ";
      $command .= "-h " . $db_spec['host'] . " ";
      $command .= "-U " . $db_spec['username'] . " ";
      $command .= $db_spec['database'] . " > " . $dump_file;
    }
    else {
      return 0;
    }
  }
  sitespinner_exec($command);
  return 1;
}

// }}}
// {{{ sitespinner_load_sql($db_spec)

/**
 * @param
 *   array  $db_spec array holding information about the database to load the sql dump into
 * @param
 *   string $dump_file, the string containing the path to the sql dump to be loaded
 * @return
 *   int on success and 0 on failure
 */
function sitespinner_load_dump($db_spec, $dump_file) {

  if ($db_spec['driver'] == 'mysql' || $db_spec['driver'] == 'mysqli') {
    $command = "mysql -u " . $db_spec['username'] . " ";
    $command .= "-p" . $db_spec['password'] . " ";
    $command .= "-h" . $db_spec['host'] . " ";
    $command .= $db_spec['database'] . " < " . $dump_file;
  }
  else {
    if ($db_spec['driver'] == 'pgsql') {
      $command = "export PGPASSWORD='$db_spec[password]';";
      $command .= "psql ";
      $command .= "-h " . $db_spec['host'] . " ";
      $command .= "-U " . $db_spec['username'] . " ";
      $command .= $db_spec['database'] . " < " . $dump_file;
    }
    else {
      sitespinner_print(dt("Unknown driver") . "\n");
      return 0;
    }
  }
  sitespinner_exec($command);
  return 1;
}

// }}}
// {{{ sitespinner_load_db($site, $db_spec, $install)
/**
 * function for populating the newly created database through
 * readline interface
 *
 * @param
 *   string $site the drupal website to enable
 * @param
 *   array $db_spec an array holding the db_spec for the database to be loaded
 * @param
 *   int  $install, indicating if it is an install or not
 * @return
 *   int 1 no loaded db (continue with manual install on web)
 *       0 is failure and
 *       2 successfull loaded database
 */
function sitespinner_load_db($site, $db_spec, $install = 0) {
  // check to see if user wants to use an sql dump.
  // if 'Yes' get a list of available dumps to use, if 'no' go to web install
  $readline_str = dt("Load a sql dump into database: ") . $db_spec['database'] . dt(" [Y/n]? ");
  $db_use_dump = strtoupper(sitespinner_readline($readline_str));

  if ($db_use_dump == 'Y' || empty($db_use_dump)) {
    $manual = 0;
    // if we are installing only option will be from a file
    if (!$install) {
      $readline_str = dt("Do you want to use an sql dump from a running drupal [s]ite or dump from a [f]ile [s/f]? ");
      $file_or_site = strtoupper(sitespinner_readline($readline_str));
    }
    else {
      // we presume when making an install there is no active drupal site
      $file_or_site = 'F';
    }
    // type of dump to read - From a file located in the commands/sitespinner/sql folder
    switch ($file_or_site) {
      case 'F' :
        // use a file for generating database

        $res = sitespinner_load_db_file($site, $db_spec);
        if (!$res) {
          return 0;
        }
        else {
          if (SITESPINNER_DEBUG) {
            print "Returning 2\n";
          }
          return 2;
        }
        break;
      // or use a sites sql as a dump for the new site.
      // if we are cloning a site, then all the sql can be
      // imported. 
      case 'S' :
        $res = sitespinner_load_db_site($site, $db_spec);
        if (!$res) {
          return 0;
        }
        else {
          return 2;
        }
        break;

      default:
        // wrong option start over
        if (SITESPINNER_DEBUG) {
          print "else Returning 0\n";
        }
        return 0;
        break;
    }
  }
  // no db loaded, it need to be a manual install
  return 1;
}

// manual install
// }}}
// {{{ sitespinner_download_dumps

/**
 * function sitespinner_download_dumps
 * just download some default sql dumps from a default
 * location if sql folder is empty
 *
 * @return
 *   int  positive on success and 0 on failure
 */
function sitespinner_download_dumps() {

  $url = "http://www.os-cms.net/sites/default/files";
  $file = "sql";
  $filetype = "tar.gz";
  $filelong = $file . "." . $filetype;
  $url_file = $url . "/" . $filelong;
  $sql_dir = dirname(__FILE__) . "/sql";

  #sitespinner_print($url_file. "\n");
  #$dl_dumps = strtoupper(sitespinner_readline("Download and install default dumps (236 KB) for all versions of drupal [Y/n]? "));
  #if ($dl_dumps == 'Y' || empty($dl_dumps)){      
  system("wget " . $url_file);
  system("wget " . $url_file . ".md5");
  sitespinner_print(dt("Md5 validating of file ") . $filelong . "\n");
  sitespinner_exec("md5sum -c " . $filelong . ".md5");
  sitespinner_exec("tar xvfz " . $filelong);
  sitespinner_exec("cp -Rf ./sql/* " . $sql_dir);
  sitespinner_exec("rm " . $filelong);
  sitespinner_exec("rm -Rf " . "./" . $file);
  sitespinner_exec("rm " . $filelong . ".md5");
  sitespinner_print(dt("sql dumps downloaded and installed") . "\n");
  #} 

}

// }}}
// {{{ sitespinner_load_db_file($site, $db_spec)

/**
 * function for loadig database throug a file method
 *
 * @param
 *   string $site the drupal website to enable
 * @param
 *   array $db_spec an array holding the db_spec specifing the database to load
 * @return
 *   int 1 on succes 0 on failure
 */
function sitespinner_load_db_file($site, $db_spec) {
  // get dump dir    
  $dump_dir = dirname(__FILE__) . "/sql";
  $res = sitespinner_list_sql_dumps(TRUE);
  if (!$res) {
    sitespinner_print(dt("Continue with clean install. No dumps were found in.  ") . $dump_dir . "\n");
    return 1;
    /*
    sitespinner_print(dt("You can choose to download some default dumps found at:" . "\n"));
    $dl_dumps = strtoupper(sitespinner_readline("Download and install default dumps (236 KB) for all versions of drupal [Y/n]? "));
    if ($dl_dumps == 'Y' || empty($dl_dumps)){      
      $dl = sitespinner_download_dumps();
    }*/
  }

  $res = sitespinner_list_sql_dumps(TRUE);
  if (!$res) {
    return 0;
  }

  sitespinner_print(dt("You can populate your database with the following sql dumps:") . "\n");
  sitespinner_list_sql_dumps();

  // dumps are found we prompt user to load a dump
  $sql_file = sitespinner_readline(dt("Enter exact name of sql file to use: "));

  if (empty($sql_file)) {
    // if the user entered a dump that did not exist, we start over again
    sitespinner_print(dt("You need to enter a file name.") . "\n");
    return 0;
  }
  $sql_file = $dump_dir . "/" . $sql_file;
  if (!file_exists($sql_file)) {
    // if the user entered a dump that did not exist, we start over again
    sitespinner_print(dt("File does not exist or is not readable. Try again. ") . "\n");
    return 0;
  }
  // got so far. time to load the dump into the database
  $res = sitespinner_load_dump($db_spec, $sql_file);
  if (!$res) {
    sitespinner_print(dt("Could not load dump. ") . $sql_file . dt(" We try again") . "\n");
    return 0;
  }
  else {
    return 1;
  }
}

// }}}
// {{{ sitespinner_load_db_site($site, $db_spec)
/**
 * function for populating database with site method
 *
 * @param
 *   string   $site the drupal website to enable
 * @param
 *   array    $db_spec an array holding the db_spec specifing the database to load
 * @return
 *   int      1 on succes 0 on failure
 */
function sitespinner_load_db_site($site, $db_spec) {

  sitespinner_print(dt("Listing all sites except: ") . "$site\n");
  sitespinner_list_all_sites($site);
  $readline_str = dt("Enter exact site name to load dump from:") . "\n";
  $load_site = sitespinner_readline($readline_str);

  if (empty($load_site)) {
    sitespinner_print(dt("You need to enter a site.") . "\n");
    return 0;
  }

  if ($load_site == $site) {
    sitespinner_print(dt("Can not load anything from the created database for the site we are enabling.") . "\n");
    return 0;
  }
  if (!sitespinner_site_exists($load_site, '')) {
    sitespinner_print(dt("The site to load from does not exist.") . "\n");
    return 0;
  }

  if (sitespinner_sub_exists($load_site)) {
    $settings = sitespinner_get_filenames($load_site);
  }
  else {
    $settings = sitespinnersa_get_filenames($load_site);
  }

  $driver_spec = 0;
  $save_spec = _drush_sql_get_spec_from_settings($settings['settings_file'], 'source');

  // check and see if we load and save from same database type
  if ($save_spec['driver'] != $db_spec['driver']) {
    $driver_spec = 0;
  }
  else {
    $driver_spec = 1;
  }
  if ($save_spec['driver'] == 'mysql' && $db_spec['driver'] == 'mysqli') {
    $driver_spec = 1;
  }
  if ($save_spec['driver'] == 'mysqli' && $db_spec['driver'] == 'mysql') {
    $driver_spec = 1;
  }

  if ($driver_spec) {
    sitespinner_save_dump($save_spec, './dump_file');
    $res = sitespinner_load_dump($db_spec, './dump_file');
    sitespinner_exec("rm ./dump_file");
    return 1;
  }
  else {
    sitespinner_print(dt("The database source and target have to be same type.") . "\n");
    return 0;
  }
  return 0;
}

// }}}
// {{{ sitespinner_list_sql_dumps()
/**
 * function for listing all sql dumps
 * @param bool is this is true no printing will be done
 * @param int major_version. If this is set only list the specified version
 * @return int $num_dumps, the count of sql dumps in the folder
 */
function sitespinner_list_sql_dumps($silent = FALSE, $major_version = '') {
  $sql_dir = dirname(__FILE__) . "/sql";
  $sql_dumps = scandir($sql_dir);
  $num_dumps = 0;
  for ($i = 0; $i < count($sql_dumps); ++$i) {
    if (!is_file($sql_dir . "/" . $sql_dumps[$i])) {
      // skip
    }
    else {
      $num_dumps++;
      if (!$silent) {
        sitespinner_print($sql_dumps[$i] . " ");
      }
    }
  }
  if (!$silent) {
    sitespinner_print("\n");
  }
  return $num_dumps;
}

// }}}
// cron function
// {{{ sitespinner_create_cron_line($site)

/**
 * function for showing a readline form where we can collect
 * information about how crons should be done
 *
 * @param string $site
 * @return array $cron with info about the cron
 */
function sitespinner_create_cron_line($site) {
  $readline_str = dt("Shall cron run once every [h]our or once every [d]ay or once every month[m]: [h]/[d]/[m]? ");
  $cron = array();
  $h_or_d = sitespinner_readline($readline_str);
  if ($h_or_d != 'h' AND $h_or_d != 'd' AND $h_or_d != 'm') {
    return 0;
  }

  $readline_str = dt("Enter minute when your cron shall run. Must be between 0 and 59: ");
  $cron = array();
  $minute = (int) sitespinner_readline($readline_str);
  if ($minute >= 0 && $minute <= 59) {
    $cron['minute'] = $minute;
  }
  else {
    return 0;
  }

  if ($h_or_d == 'h') {
    return $cron;
  }

  $readline_str = dt("Enter hour when your cron shall run. Must be between 0 and 23: ");
  $hour = (int) sitespinner_readline($readline_str);
  if ($hour >= 0 && $hour <= 23) {
    $cron['hour'] = $hour;
  }
  else {
    return 0;
  }

  if ($h_or_d == 'd') {
    return $cron;
  }

  $readline_str = dt("Enter day in month when your cron shall run. Must be between 1 and 31: ");
  $day = (int) sitespinner_readline($readline_str);
  if ($day >= 1 && $day <= 31) {
    $cron['day'] = $day;
  }
  else {
    return 0;
  }

  if ($h_or_d == 'm') {
    return $cron;
  }

  return 0;
}

// }}} 
// {{{ sitespinner_disable_cron($site)

/**
 * function for deleting entries from the crontab file
 *
 * @param string $site
 * @return int 1
 */
function sitespinner_disable_cron($site) {

  $cron_line = "http://" . $site . "/cron.php";
  $output = $cron_str = '';

  // get contents of crontab. We use exec because we need the output
  // do the crontab -l command to get old crontabs
  exec("crontab -l", $output, $res);
  foreach ($output as $line) {
    if (!preg_match("[$cron_line]", $line)) {
      $cron_str .= $line . "\n";
    }
  }

  $cron_str = rtrim($cron_str);
  $command = 'echo "' . $cron_str . '" | crontab -';
  sitespinner_exec($command);
  return 1;
}

// }}}
// {{{ sitespinner_enable_cron($site)
/**
 * function for enabling the site in the crontab
 *
 * @param string $site
 * @return int 0 on failure 1 on success
 */
function sitespinner_enable_cron($site) {
  $readline_str = dt("Do you want to enable cron job for the site [Y/n]? ");
  $y_or_n = strtoupper(sitespinner_readline($readline_str));
  if ($y_or_n == 'Y' || empty($y_or_n)) {
    // continue
  }
  else {
    return 0;
  }

  $cron = sitespinner_create_cron_line($site);
  if (!$cron) {
    sitespinner_print("Could not enable cron");
    return 0;
  }
  else {
    // minute always set in this scenario
    $cron_line = $cron['minute'] . " ";
    // but not the day
    if (isset($cron['hour'])) {
      $cron_line .= $cron['hour'] . " ";
    }
    else {
      $cron_line .= "* ";
    }
    // and not the month
    if (isset($cron['day'])) {
      $cron_line .= $cron['day'] . " ";
    }
    else {
      $cron_line .= "* ";
    }
    // add the remains
    $cron_line .= "* *  ";
  }

  $cron_line .= SITESPINNER_CRON_COMMAND . " " . "http://" . $site . "/cron.php";

  // get contents of crontab. We use exec because we need the output
  // do the crontab -l command to get old crontabs
  exec("crontab -l", $output, $res);
  $output = sitespinner_print($output, 1);
  $output .= $cron_line;
  $command = 'echo "' . $output . '" | crontab -';
  sitespinner_exec($command);

  return 1;
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
// {{{ sitespinner_list_all_sites()

/**
 * @param   string $exclude , an optional site to exclude form the search
 *          We only operate at a site at a time so nice to be
 *          able to exclude a site from listings etc.
 *
 *   function for listing all the sites in the sites/ directory
 */
function sitespinner_list_all_sites($exclude = '') {
  $res = sitespinner_init();
  if (!$res) {
    return $res;
  }

  $all_sites = array();
  $all_sites = sitespinner_get_all_sites($exclude);
  foreach ($all_sites as $site) {
    sitespinner_print($site['base'] . " serves: ");
    foreach ($site['sub'] as $sub_site) {
      sitespinner_print($sub_site . " ");
    }
    sitespinner_print("\n");
  }
}

// }}}
// {{{ sitespinner_list_sub_sites()

/**
 * function for listing all the sites in the sites/ directory
 */
function sitespinner_list_sub_sites() {
  $sub_sites = array();
  $sub_sites = sitespinner_get_sub_sites();
  foreach ($sub_sites as $site) {
    sitespinner_print($site . " "); // " has the following sub: \n";
  }
  sitespinner_print("\n");
}

// }}}
// {{{ sitespinner_get_all_sites()

/**
 * @param
 *   string $exclude, an optional site to exclude form the search
 *   We only operate at a site at a time so nice to be
 *   able to exclude a site from listings etc.
 * @return
 *   array $sites an array with base sites containing an array of all
 *   the base sites sub sites
 */
function sitespinner_get_all_sites($exclude = '') {
  // get all base sites
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $drupal_sites_path = sitespinner_get_sites_root($drupal_root);
  $sites = array();

  $sites_dir = scandir($drupal_sites_path);
  for ($i = 0; $i < count($sites_dir); ++$i) {
    $settings_file = $drupal_sites_path . "/" . $sites_dir[$i] . "/" . SITESPINNER_APACHE_HTDOCS . "/sites/default/settings.php";

    if ($sites_dir[$i] == '.') {
      // skip current dir
    }
    else {
      if (!is_dir($drupal_sites_path . "/" . $sites_dir[$i])) {
        // skip if not a dir
      }
      else {
        if (!file_exists($settings_file)) {
          // skip if no default settings file was found
        }
        else {
          if ($sites_dir[$i] == '..') {
            // skip if it is base dir
          }
          else {
            if ($sites_dir[$i] == 'default') {
              // skip if it is default dir
            }
            else {
              if ($sites_dir[$i] == 'all') {
                // skip if dir is all
              }
              else {
                if ($sites_dir[$i] == $exclude) {
                  // skip if dir should be excluded
                }
                else {
                  // grab the base if it is a base
                  $sites[$i]['base'] = $sites_dir[$i];
                  $sites[$i]['sub'] = sitespinner_get_sub_sites($sites_dir[$i], $exclude);
                }
              }
            }
          }
        }
      }
    }
  }
  return $sites;
}

// }}}
// {{{ sitespinner_get_base_site($sub);

/**
 * function for getting a base site to a sub-site
 *
 * @param string $sub name of the sub-site
 * @return mixed
 *  string $base name of the base site or
 *  0 if no base site could be found
 */
function sitespinner_get_base_site($sub) {

  $all_sites = array();
  $all_sites = sitespinner_get_all_sites();
  foreach ($all_sites as $site) {
    foreach ($site['sub'] as $sub_site) {
      $base = $site['base'];
      if ($sub_site == $sub) {
        return $base;
      }
    }
  }
  return 0;
}

// }}}
// {{{ sitespinner_is_current_base($site)

/**
 * function to determinate if the site where we execute the sitespinner script
 * is the actual base site
 * @param string $base the site to examine
 */

// FIX: Do we use this function
function sitespinner_is_current_base($base) {
  $settings_file = $_SERVER['PWD'] . "/sites/default/settings.php";
  if (file_exists($settings_file)) {
    return 1;
  }
  else {
    return 0;
  }
}

// }}}
// {{{ sitespinner_sub_to_current($site)

/**
 * function to determine if a site is sub to current site
 * @param string $sub the sub-site
 * @return int 1 if it is and 0 if it is not
 */
function sitespinner_sub_to_current($sub) {
  $settings_dir = drush_get_context('DRUSH_DRUPAL_ROOT') . "/sites/" . $sub;
  if (file_exists($settings_dir)) {
    return 1;
  }
  else {
    return 0;
  }

}

// }}}
// {{{ sitespinner_site_exists ($site, $exclude)

/**
 * function for determing if a site exists
 *
 * @param string $site the site to examine
 * @param string $exclude a site to exclude
 * @return int 1 if it exists and 0 if it does not
 */
function sitespinner_site_exists($site, $exclude) {
  if (sitespinner_base_exists($site, $exclude)) {
    return 1;
  }
  if (sitespinner_sub_exists($site, $exclude)) {
    return 1;
  }
  return 0;
}

// }}}
// {{{ sitespinner_base_exists($site)


/**
 * function for determing if a base site exists
 *
 * @param string $site the site to examine
 * @param string $exclude a site to exclude
 * @return int 1 if it exists and 0 if it does not
 */
function sitespinner_base_exists($base, $exclude = '') {
  $all_sites = array();
  $all_sites = sitespinner_get_all_sites();
  $exists = 0;
  foreach ($all_sites as $site) {
    foreach ($site as $base_site) {
      if ($base_site == $base) {
        $exists = 1;
      }
      if (!empty($exclude) && $base_site == $exclude) {
        $exists = 0;
      }
    }
  }
  return $exists;
}

// }}}
// {{{ sitespinner_sub_exists($site)

/**
 * function to determine if a sub-site exists
 *
 * @param string $site the site to examine
 * @param string $exclude a site to exclude
 * @return int 1 if it exists and 0 if it does not
 */
function sitespinner_sub_exists($sub, $exclude = '') {
  $all_sites = array();
  $all_sites = sitespinner_get_all_sites();
  $exists = 0;
  foreach ($all_sites as $site) {
    foreach ($site['sub'] as $sub_site) {
      if ($sub_site == $sub) {
        $exists = 1;
      }
      if (isset($exclude) && $sub_site == $exclude) {
        $exists = 0;
      }
    }
  }
  return $exists;
}

// }}}
// {{{ sitespinner_get_sub_sites($base)

/**
 * @param
 *   string $exclude, an optional site to exclude form the search
 * @return
 *   array $sites an array of all the base sites, which each can serve
 *         as multisite environments
 */
function sitespinner_get_sub_sites($base = '', $exclude = '') {
  // get local sites for the base install where you are standing 
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
  if (empty($base)) {
    $drupal_sites_path = $drupal_root . "/sites";
    $sites = array();
  }
  else {
    $sites_root = sitespinner_get_sites_root($drupal_root);
    $drupal_sites_path = $sites_root . "/" . $base . "/" . SITESPINNER_APACHE_HTDOCS . "/sites";
  }

  $sites_dir = scandir($drupal_sites_path);
  for ($i = 0; $i < count($sites_dir); ++$i) {
    $settings_file = $drupal_sites_path . "/" . $sites_dir[$i] . "/settings.php";

    if ($sites_dir[$i] == '.') {
      // skip current dir
    }
    else {
      if (!is_dir($drupal_sites_path . "/" . $sites_dir[$i])) {
        // skip if not a dir
      }
      else {
        if (!file_exists($settings_file)) {
          // skip if no settings file was found
        }
        else {
          if ($sites_dir[$i] == '..') {
            // skip if it is base dir
          }
          else {
            if ($sites_dir[$i] == 'default') {
              // skip if it is default dir
            }
            else {
              if ($sites_dir[$i] == 'all') {
                // skip if dir is all
              }
              else {
                if ($sites_dir[$i] == $exclude) {
                  // skip if dir is excluded
                }
                else {
                  // grab the dir
                  $sites[$i] = $sites_dir[$i];
                }
              }
            }
          }
        }
      }
    }
  }
  return $sites;

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
// {{{ sitespinner_test()

/**
 * function for testing
 */
function sitespinner_test($site) {
  /*
  $db_spec = array();
  $db_spec = drush_get_context('DRUSH_DB_CREDENTIALS');
  print_r($db_spec);
  $db_spec = sitespinner_get_db_spec();  
  print_r($db_spec);
   */
  sitespinner_download_dumps();
}

#$drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
/*
//$settings = sitespinner_get_filenames($site);
//print_r($settings);
require_once $drupal_root . "/includes/common.inc";
require_once $drupal_root . "/modules/system/system.install";
//print $drupal_root;
require_once $drupal_root . "/includes/install.inc";
drupal_uninstall_schema();
 */
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

function sitespinner_create_mysql_copy_command($source_db_spec, $dest_db_spec) {
  $command = dt('mysqldump -h @source_host -u @source_user -p@source_pass @source_port @source_db_name | mysql -h @destination_host -u @destination_user -p@destination_pass @destination_port @destination_db_name', array(
    '@source_host' => $source_db_spec['host'],
    '@source_user' => $source_db_spec['username'],
    '@source_pass' => $source_db_spec['password'],
    '@source_port' => !empty($source_db_spec['port']) ? '-P ' . $source_db_spec['port'] : '',
    '@source_db_name' => $source_db_spec['database'],
    '@destination_host' => $dest_db_spec['host'],
    '@destination_user' => $dest_db_spec['username'],
    '@destination_pass' => $dest_db_spec['password'],
    '@destination_port' => !empty($dest_db_spec['port']) ? '-P ' . $dest_db_spec['port'] : '',
    '@destination_db_name' => $dest_db_spec['database'],
  ));

  return $command;
}