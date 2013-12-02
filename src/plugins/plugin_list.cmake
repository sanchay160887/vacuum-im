add_subdirectory(accountmanager)
add_subdirectory(annotations)
add_subdirectory(autostatus)
add_subdirectory(avatars)
add_subdirectory(birthdayreminder)
add_subdirectory(bitsofbinary)
add_subdirectory(bookmarks)
add_subdirectory(captchaforms)
add_subdirectory(chatmessagehandler)
add_subdirectory(chatstates)
add_subdirectory(clientinfo)
add_subdirectory(commands)
add_subdirectory(compress)
add_subdirectory(connectionmanager)
add_subdirectory(console)
add_subdirectory(dataforms)
add_subdirectory(datastreamsmanager)
add_subdirectory(defaultconnection)
add_subdirectory(emoticons)
add_subdirectory(filemessagearchive)
add_subdirectory(filestreamsmanager)
add_subdirectory(filetransfer)
add_subdirectory(gateways)
add_subdirectory(inbandstreams)
add_subdirectory(iqauth)
add_subdirectory(jabbersearch)
add_subdirectory(mainwindow)
add_subdirectory(messagearchiver)
add_subdirectory(messagecarbons)
add_subdirectory(messageprocessor)
add_subdirectory(messagestyles)
add_subdirectory(messagewidgets)
#add_subdirectory(metacontacts)
add_subdirectory(multiuserchat)
add_subdirectory(normalmessagehandler)
add_subdirectory(notifications)
add_subdirectory(optionsmanager)
add_subdirectory(pepmanager)
add_subdirectory(presence)
add_subdirectory(privacylists)
add_subdirectory(privatestorage)
add_subdirectory(recentcontacts)
add_subdirectory(registration)
add_subdirectory(remotecontrol)
add_subdirectory(roster)
add_subdirectory(rosterchanger)
add_subdirectory(rosteritemexchange)
add_subdirectory(rostersearch)
add_subdirectory(rostersmodel)
add_subdirectory(rostersview)
add_subdirectory(saslauth)
add_subdirectory(servermessagearchive)
add_subdirectory(servicediscovery)
add_subdirectory(sessionnegotiation)
add_subdirectory(shortcutmanager)
add_subdirectory(simplemessagestyle)
add_subdirectory(socksstreams)
add_subdirectory(spellchecker)
add_subdirectory(stanzaprocessor)
add_subdirectory(starttls)
add_subdirectory(statistics)
add_subdirectory(statuschanger)
add_subdirectory(statusicons)
add_subdirectory(traymanager)
add_subdirectory(urlprocessor)
add_subdirectory(vcard)
add_subdirectory(xmppstreams)
add_subdirectory(xmppuriqueries)

if (QT_QTWEBKIT_LIBRARY)
  add_subdirectory(adiummessagestyle)
endif (QT_QTWEBKIT_LIBRARY)
