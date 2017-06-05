# Sitespinner

Sitespinner provides two drush commands: sitespinner (ss), and sitespinner-delete (ssd).

1. **sitespinner** creates a new Drupal multisite based on an existing live template site.
It uses drush site alias files to describe the source (template) and destination (new) sites.
Sitespinner copies the template site's database and files directory to the new destination site, and then writes any variables specified in the destination alias to the destination site, overriding the original values. This allows you to maintain one live, canonical template site (we call ours: "templatesite") from which all new sites will be based.

2. **sitespinner-delete** simply deletes a multisite based on the specific site alias you provide. It will permanently delete the site's files (sites/{site root} directory), symlink, and corresponding MySQL database and tables.

## Getting Started

### Prerequisites

* [drush-All-Versions-2.0.tar.gz](http://ftp.drupal.org/files/projects/drush-All-Versions-2.0.tar.gz) or any later version should work
* a working Drupal site that either already is functioning as a multisite, or that you want to function as a multisite
* PHP, Apache2, MySQL or PostgreSQL
* sudo (and root privileges) (on other Debian you may need to do: ```su root```
* permissions to create databases (MySQL or PostgreSQL)

### Install Sitespinner

* First, locate your server's "drush" executable file
  * ```which drush```
* Install sitespinner to the "drush/commands" folder
  * ```cd /opt/drush-6.x/vendor/drush/drush/commands```
  * ```git clone https://github.com/Islandora-Collaboration-Group/drush-sitespinner.git sitespinner```
* Copy (and rename) the sample alias file to /home/islandora/.drush/ (alias files are typically installed at ~/.drush/)
  * ```cp sitespinner/examples/sitespinner-sample.aliases.drushrc.php /home/islandora/.drush/sitespinner-peace.aliases.drushrc.php```
  * Tip: create an alias file for each new site (i.e. sitespinner-economics.aliases.drushrc.php, sitespinner-faculty.aliases.drushrc.php)
* Edit "Configuration Settings" in your renamed alias file to define the source and destination aliases
  * ```cd /home/islandora/.drush```
  * ```emacs sitespinner-peace.aliases.drushrc.php```

### Create a new Drupal multi-site
* Once source and destination aliases are configured, specify your "source" and "destination" aliases:
  * ```drush sitespinner @sitespinner-sample.template @sitespinner-sample.peace```
* The command above will create a new multi-site at: https://subdomain.example.edu/peace
* shortcut: "ss" = "sitespinner"

### Delete one specific preexisting Drupal multi-site

* You must specify your "destination" alias
  * ```drush sitespinner-delete @sitespinner-sample.peace```
* The drush command above will permanently delete the site's files (sites/{site root} directory), symlink, and corresponding MySQL database and tables
* shortcut: "ssd" = "sitespinner-delete"

### Important Final Steps

* Be sure to edit the "filter-drupal.xml" file on the Fedora server to add or remove this site (see [duraspace documentation](https://wiki.duraspace.org/pages/viewpage.action?pageId=69833569))
  * ```cd /usr/local/fedora/server/config/```
* Restart tomcat:
  * ```sudo su```
  * ```service tomcat restart```
  * ```exit```

### Options

You can use the following flags:
* ```--simulate``` See what the commands will do to your system. When using simulate there will be no changes to your system.
* ```-f``` Force the script to continue when errors are encountered. E.g.: You have tried to install a site but your failed halfway due to permissions. You can then change the permissions on your server, and force the script to execute.
* ```-s``` Silence the script will produce less output, and only prompt you when questions are raised.
* ```-d``` Show verbose debugging output

### Useful drush commands

* ```drush sa```                          [list all site aliases] [shortcut: "sa" = "site-alias"]
* ```drush sa @sitespinner-sample```      [shows execution view of a specific alias file, without any actual execution]
* ```drush topic docs-aliases```          [documentation on writing and installing drush site aliases]
* ```drush help sa```                     [documentation for site alias]
* ```drush help```                        [documentation for drush]
* ```drush help sitespinner```            [documentation for sitespinner create]
* ```drush help sitespinner-delete```     [documentation for sitespinner delete]
* ```drush cc drush```                    [clear drush cache] [shortcut: "cc" = "cache-clear"]
* ```drush topic docs-aliases```          [helpful information on writing and installing drush site aliases]
* [more drush help](https://github.com/drush-ops/drush/blob/master/examples/example.aliases.drushrc.php)

## More information about alias files

#### Template alias

To be used as a source alias with sitespinner, the source alias file, at a minimum, needs to provide values for these:

    ['root'] (The Drupal root)
    ['uri']
    ['db_url'] or ['databases']
    ['path-aliases']['%files']

#### Destination alias
To be used as a destination site alias, the file needs to provide, at a minimum, values for these:

    ['root'] (This is the drupal root, not the path to the actual multisite's directory under /sites/.)
    ['uri']
    ['databases']['default']['default'] (IMPORTANT: ['db_url'] will not work!!!!)
    ['path-aliases']['%files']

#### Other sitespinner specific values that you should normally provide:

Since you will be creating a database, you will probably need to provide a mysql username and password for a more privileged user than your standard drupal database user. If not provided, then this command will attempt to create the database using the default drupal database connection info.

    ['sitespinner-destination']['db_creator'] = array(
      'username' => "...",
      'password' => "...",
    );

A path to a file to use as a settings.php template. It must contain a line with the text  "$databases = array();". If not provided, the /sites/default/default.settings.php file will be used.

    ['sitespinner-destination']['settings_file_template']

Users, groups, and permissions for the files and directories that will be created.

    ['sitespinner-destination']['server-environment'] = array(
        'default_user' => "...",
        'default_group' => "...",
        'settings_file_permissions' => '640',
        'files_directory_permissions' => '770',
    );

Read more notes in the sample aliases file.

    ['sitespinner-destination']['create-domain'] = array(
        'type' => 'path',
        'name' => '...',
    );

Variable overrides. Any key => value pairs that you put into the variables array will be written to the variables table in the destination site's database. In this way you can specify the site name, theme settings, and many other configuration options. Any array values will be recursively merged with values provided in inherited (parent) alias files.

    ['sitespinner-destination']['variables'] = array(
        'key' => 'value',
        'another key' => array(
            'cool' => 'I can write arrays to the variables table!'
        ),
    );

#### Parent alias
A little known feature of drush site aliases is that you can specify a comma-separated list of site aliases whose properties will be inherited by this alias. To do so, add a 'parent' key to the alias array:

    'parent' => '@etc, @grandparent, @parent',

This can be handy when managing multiple configurations of sites in your multisite setup. You can define the characteristics that are common to a given configuration in the parent alias, and then in the child site, just provide those details that are needed for that site.


## Contributing

Contact the authors if you want to submit pull requests to us.

## Authors

* **Chris Warren** - *Initial version of Sitespinner in PHP*
* **Pat Dunlavey** - *Refactored to use drush*
* **David Keiser-Clark** - *Bug fixes, readme, working samples* - [dwk2](https://github.com/dwk2)

## License

This project is licensed under the GNU GENERAL PUBLIC LICENSE - see the [LICENSE.txt](LICENSE.txt) file for details
