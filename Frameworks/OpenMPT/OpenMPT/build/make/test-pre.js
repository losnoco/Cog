
var Module = {
 'preInit': function(text) {
  FS.mkdir('/test');
  FS.mount(NODEFS, {'root': '../test/'}, '/test');
  FS.mkdir('/libopenmpt');
  FS.mount(NODEFS, {'root': '../libopenmpt/'}, '/libopenmpt');
 }
};
