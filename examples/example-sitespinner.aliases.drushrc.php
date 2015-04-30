<?php

/**
 * @file
 * example-sitespinner.aliases.drushrc.php
 *
 * This file provides examples of how drush site aliases can be used in
 * sitespinner.
 *
 * For information on writing and installing drush site aliases: type
 *      drush topic docs-aliases
 * on the command line.
 *
 * To use the site aliases as configured in this file to create a new multisite,
 * you would type
 *      drush sitespinner @template @new
 * This would create a new multisite at example.com/new, based on the
 * configuration provided here.
 */

/**
 * This alias provides the mimimum needed to be able to use "@template" as a
 * source site in sitespinner.
 *
 * Note that this site must actually exist!
 */
$aliases['template'] = array(
  // "root" is where the index.php file is located, not the multi-site root.
  'root' => '/var/www/html',
  'uri' => 'example.com/template',
  'db-url' => 'mysql://drupal_user:drupal_password@localhost/template_db',
  'path-aliases' => array(
    // REQUIRED: %files path alias on source is required for sitespinner.
    '%files' => '/var/www/html/sites/example.com.template/files',
  ),
);

/**
 * Drush aliases can inherit properties from parent aliases. Here we are
 * creating an alias called "parent" solely for the purpose of providing
 * onfiguration that can be common to multiple "child" sites. Note that since
 * the only purpose of this alias is to provide values that children may
 * inherit, we don't need to provide everything needed for this to be a
 * functional alias. For example, there is no 'uri' value below.
 */
$aliases['parent'] = array(

  // In a multisite, the root is the same for all sites. So we define it here,
  // in the parent alias.
  'root' => '/var/www/html',

  /*
   * The "sitespinner-destination" array provides a variety of sitespinner-
   * specific information. When inherited from a parent alias, the child
   * sitespinner-destination array will be recursively merged to the parent
   * array.
   */
  'sitespinner-destination' => array(

    // Mysal username and password with database creation privileges. If not
    // provided, the username and password for the drupal site will be used.
    'db_creator' => array(
      'username' => "master_of_the_universe",
      'password' => "correspondingly_cool_password",
    ),

    // Path to a template settings file. There must be a line...
    // $databases = array();
    // ...in that file.
    // This will be replaced by the actual database connection information after
    // the file is copied to the new site's root directory.
    'settings_file_template' => '/var/www/html/sites/example.com.template/default.settings.php',

    // Information about the server environment.
    'server-environment' => array(
      'default_user' => getenv('USER'),
      'default_group' => '_www',
      'settings_file_permissions' => '640',
      'files_directory_permissions' => '770',
    ),

    // Values that will be pushed into the variables table of the destination
    // site.
    'variables' => array(
      'site_name' => 'Parent Site',
      'islandora_paged_content_djatoka_url' => 'http://example.com/adore-djatoka/resolver',
      'islandora_base_url'                  => "http://example.com:8080/fedora",
      'islandora_solr_url'                  => "example.com:8080/solr",
      'imagemagick_convert'                 => "/usr/local/bin/convert",
      'islandora_batch_java'                => "/usr/bin/java" ,
      'islandora_lame_url'                  => "/usr/local/bin/lame",
      'islandora_paged_content_gs'          => "/usr/bin/gs",
      'islandora_video_ffmpeg_path'         => "/usr/bin/ffmpeg",
      'islandora_video_ffmpeg2theora_path'  => "/usr/bin/ffmpeg2theora",
      'islandora_kakadu_url'                => "/usr/local/bin/kdu_compress",
      'islandora_pdf_path_to_pdftotext'     => "/usr/bin/pdftotext",
      'islandora_fits_executable_path'      => "/opt/fits/fits.sh",
      'theme_tao_settings' => array(
        "theme_settings" => "",
        "toggle_logo" => 0,
        "toggle_name" => 0,
        "toggle_slogan" => 0,
        "toggle_node_user_picture" => 0,
        "toggle_comment_user_picture" => 0,
        "toggle_comment_user_verification" => 0,
        "toggle_favicon" => 0,
        "toggle_main_menu" => 0,
        "toggle_secondary_menu" => 0,
        "logo" => "",
        "default_logo" => 0,
        "logo_path" => "",
        "logo_upload" => "",
        "default_favicon" => 0,
        "favicon_path" => "public://home-logo_4.jpg",
        "favicon_upload" => "",
        "project_header_image_fid" => "",
        "background_image_folder" => "gray-glow",
        "mission_statement" => array(
          "value" => '',
          "format" => "filtered_html",
        ),
        "favicon_mimetype" => "image/jpeg",
      ),
    ),
  ),
);


/**
 * And here is an example of a destination alias. We have to provide all the
 * mandatory site alias information (see comments below) either here, or via
 * inheritance from a parent.
 */
$aliases['new'] = array(

  // Here, we are chosing to inherit configuration from the 'parent'
  // alias.
  'parent' => '@parent',

  // Generally, for a destination site alias file, you will need, at a minimum,
  // the following, either specified in this alias, or inherited from a parent
  // alias:
  // ['root']
  // ['uri']
  // ['path-aliases']['%files']
  // ['databases']['default']['default']
  'uri' => 'williams-islandora.local/new',
  'path-aliases' => array(
    '%dump' => '/Users/pat/Sites/db_dumps/unbound-new.sql',
    // REQUIRED: %files path alias on destination is required for sitespinner.
    '%files' => '/Users/pat/Sites/CommonMedia/williams-islandora/site/sites/williams-islandora.local.new/files',
  ),

  // A complete database definition array is required for sitespinner.
  // This will be written as-is to the $databases variable in
  // the settings.php file,  which will be created (if it doesn't
  // already exist).
  'databases' => array(
    'default' => array(
      'default' => array(
        'database' => 'example_new',
        'username' => 'drupal_user',
        'password' => 'drupal_password',
        'host' => 'localhost',
        'port' => '',
        'driver' => 'mysql',
        'prefix' => '',
      ),
    ),
  ),

  'sitespinner-destination' => array(

    // Define the way the site will be instantiated as a multisite,
    //
    // 'type' maybe be one of either 'path', 'domain', or 'subdomain':
    // -- 'path' means that the site will be accessed using a path alias under
    // the default site's domain. E.g. domain.com/path
    // -- 'domain' means that the site will have it's own domain name
    // E.g. domain2.com
    // -- 'subdomain' means that the site will be created as a subdomain of the
    // default site's domain. E.g. subdomain.domain.com
    //
    // 'name' defines the name to use for the given domain type.
    'create-domain' => array(
      'type' => 'path',
      'name' => 'new',
    ),

    // Values that will be pushed into the variables table. (These are merged
    // with the array inherited from the parent alias.)
    'variables' => array(
      'site_name' => 'New Site',
      'theme_tao_settings' => array(
        "background_image_folder" => "orange-glow",
        "mission_statement" => array(
          "value" => 'This site breaks the bonds of traditional publishing by providing a platform for collecting, preserving, sharing and building upon our creative, historical and scholarly works.',
        ),
      ),
    ),
  ),
);
