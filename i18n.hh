#ifndef PDF2DJVU_I18N
#define PDF2DJVU_I18N

static inline const char * ngettext(const char *message_id, const char *message_id_plural, unsigned long int n)
{
  return n == 1 ? message_id : message_id_plural;
}

static inline const char * _(const char *message_id)
{
  return message_id;
}

static inline void initialize_i18n()
{
  /* Do nothing. */
}

#endif

// vim:ts=2 sw=2 et
