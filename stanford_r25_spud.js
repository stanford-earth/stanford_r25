(function ($) {

    // javascript to display calendar and control embeds (spuds) from 25Live Publisher

    // get the webname of the "spud" from Drupal
    $(document).ready(function () {

        if (typeof Drupal.settings.stanfordR25Room !== 'undefined') {

            var stanford_r25_room = Drupal.settings.stanfordR25Room;
            var stanford_r25_webname = Drupal.settings.stanfordR25Spud;

            $Trumba.addSpud({
                webName: stanford_r25_webname,
                spudType: "chooser",
                spudId: "control-spud"
            });

            $Trumba.addSpud({
                webName: stanford_r25_webname,
                spudType: "main",
                spudId: "calendar-spud"
            });

        }
    });

})(jQuery);
