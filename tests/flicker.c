#include <ctk/ctk.h>

CtkWidget*
create_flicker (void)
{
  CtkWidget *window1;
  CtkWidget *hpaned1;
  CtkWidget *vpaned2;
  CtkWidget *hbox2;
  CtkAdjustment *spinbutton7_adj;
  CtkWidget *spinbutton7;
  CtkAdjustment *spinbutton8_adj;
  CtkWidget *spinbutton8;
  CtkWidget *vbox1;
  CtkAdjustment *spinbutton9_adj;
  CtkWidget *spinbutton9;
  CtkAdjustment *spinbutton10_adj;
  CtkWidget *spinbutton10;
  CtkAdjustment *spinbutton11_adj;
  CtkWidget *spinbutton11;
  CtkAdjustment *spinbutton12_adj;
  CtkWidget *spinbutton12;
  CtkAdjustment *spinbutton13_adj;
  CtkWidget *spinbutton13;
  CtkAdjustment *spinbutton14_adj;
  CtkWidget *spinbutton14;
  CtkAdjustment *spinbutton15_adj;
  CtkWidget *spinbutton15;
  CtkAdjustment *spinbutton16_adj;
  CtkWidget *spinbutton16;
  CtkWidget *vpaned1;
  CtkWidget *hbox1;
  CtkAdjustment *spinbutton17_adj;
  CtkWidget *spinbutton17;
  CtkAdjustment *spinbutton18_adj;
  CtkWidget *spinbutton18;
  CtkAdjustment *spinbutton19_adj;
  CtkWidget *spinbutton19;
  CtkWidget *vbox2;
  CtkAdjustment *spinbutton20_adj;
  CtkWidget *spinbutton20;
  CtkAdjustment *spinbutton21_adj;
  CtkWidget *spinbutton21;
  CtkAdjustment *spinbutton22_adj;
  CtkWidget *spinbutton22;
  CtkAdjustment *spinbutton23_adj;
  CtkWidget *spinbutton23;
  CtkAdjustment *spinbutton24_adj;
  CtkWidget *spinbutton24;
  CtkAdjustment *spinbutton25_adj;
  CtkWidget *spinbutton25;
  CtkAdjustment *spinbutton26_adj;
  CtkWidget *spinbutton26;
  CtkAdjustment *spinbutton27_adj;
  CtkWidget *spinbutton27;

  window1 = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_default_size (CTK_WINDOW (window1), 500, 400);
  ctk_window_set_title (CTK_WINDOW (window1), "window1");

  hpaned1 = ctk_paned_new (CTK_ORIENTATION_HORIZONTAL);
  ctk_widget_show (hpaned1);
  ctk_container_add (CTK_CONTAINER (window1), hpaned1);
  ctk_paned_set_position (CTK_PANED (hpaned1), 100);

  vpaned2 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_widget_show (vpaned2);
  ctk_paned_pack1 (CTK_PANED (hpaned1), vpaned2, FALSE, TRUE);
  ctk_paned_set_position (CTK_PANED (vpaned2), 100);

  hbox2 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_show (hbox2);
  ctk_paned_pack1 (CTK_PANED (vpaned2), hbox2, FALSE, TRUE);

  spinbutton7_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton7 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton7_adj), 1, 0);
  ctk_widget_show (spinbutton7);
  ctk_box_pack_start (CTK_BOX (hbox2), spinbutton7, TRUE, TRUE, 0);

  spinbutton8_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton8 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton8_adj), 1, 0);
  ctk_widget_show (spinbutton8);
  ctk_box_pack_start (CTK_BOX (hbox2), spinbutton8, TRUE, TRUE, 0);

  vbox1 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (vbox1);
  ctk_paned_pack2 (CTK_PANED (vpaned2), vbox1, TRUE, TRUE);

  spinbutton9_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton9 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton9_adj), 1, 0);
  ctk_widget_show (spinbutton9);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton9, FALSE, FALSE, 0);

  spinbutton10_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton10 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton10_adj), 1, 0);
  ctk_widget_show (spinbutton10);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton10, FALSE, FALSE, 0);

  spinbutton11_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton11 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton11_adj), 1, 0);
  ctk_widget_show (spinbutton11);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton11, FALSE, FALSE, 0);

  spinbutton12_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton12 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton12_adj), 1, 0);
  ctk_widget_show (spinbutton12);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton12, FALSE, FALSE, 0);

  spinbutton13_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton13 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton13_adj), 1, 0);
  ctk_widget_show (spinbutton13);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton13, FALSE, FALSE, 0);

  spinbutton14_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton14 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton14_adj), 1, 0);
  ctk_widget_show (spinbutton14);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton14, FALSE, FALSE, 0);

  spinbutton15_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton15 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton15_adj), 1, 0);
  ctk_widget_show (spinbutton15);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton15, FALSE, FALSE, 0);

  spinbutton16_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton16 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton16_adj), 1, 0);
  ctk_widget_show (spinbutton16);
  ctk_box_pack_start (CTK_BOX (vbox1), spinbutton16, FALSE, FALSE, 0);

  vpaned1 = ctk_paned_new (CTK_ORIENTATION_VERTICAL);
  ctk_widget_show (vpaned1);
  ctk_paned_pack2 (CTK_PANED (hpaned1), vpaned1, TRUE, TRUE);
  ctk_paned_set_position (CTK_PANED (vpaned1), 0);

  hbox1 = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
  ctk_widget_show (hbox1);
  ctk_paned_pack1 (CTK_PANED (vpaned1), hbox1, FALSE, TRUE);

  spinbutton17_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton17 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton17_adj), 1, 0);
  ctk_widget_show (spinbutton17);
  ctk_box_pack_start (CTK_BOX (hbox1), spinbutton17, TRUE, TRUE, 0);

  spinbutton18_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton18 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton18_adj), 1, 0);
  ctk_widget_show (spinbutton18);
  ctk_box_pack_start (CTK_BOX (hbox1), spinbutton18, TRUE, TRUE, 0);

  spinbutton19_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton19 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton19_adj), 1, 0);
  ctk_widget_show (spinbutton19);
  ctk_box_pack_start (CTK_BOX (hbox1), spinbutton19, TRUE, TRUE, 0);

  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (vbox2);
  ctk_paned_pack2 (CTK_PANED (vpaned1), vbox2, FALSE, FALSE);

  spinbutton20_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton20 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton20_adj), 1, 0);
  ctk_widget_show (spinbutton20);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton20, FALSE, FALSE, 0);

  spinbutton21_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton21 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton21_adj), 1, 0);
  ctk_widget_show (spinbutton21);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton21, FALSE, FALSE, 0);

  spinbutton22_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton22 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton22_adj), 1, 0);
  ctk_widget_show (spinbutton22);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton22, FALSE, FALSE, 0);

  spinbutton23_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton23 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton23_adj), 1, 0);
  ctk_widget_show (spinbutton23);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton23, FALSE, FALSE, 0);

  spinbutton24_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton24 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton24_adj), 1, 0);
  ctk_widget_show (spinbutton24);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton24, FALSE, FALSE, 0);

  spinbutton25_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton25 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton25_adj), 1, 0);
  ctk_widget_show (spinbutton25);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton25, FALSE, FALSE, 0);

  spinbutton26_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton26 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton26_adj), 1, 0);
  ctk_widget_show (spinbutton26);
  ctk_box_pack_start (CTK_BOX (vbox2), spinbutton26, TRUE, FALSE, 0);

  spinbutton27_adj = ctk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton27 = ctk_spin_button_new (CTK_ADJUSTMENT (spinbutton27_adj), 1, 0);
  ctk_widget_show (spinbutton27);
  ctk_box_pack_end (CTK_BOX (vbox2), spinbutton27, FALSE, FALSE, 0);


  return window1;
}


int
main (int argc, char *argv[])
{
  CtkWidget *window1;

  ctk_init (&argc, &argv);

  window1 = create_flicker ();
  ctk_widget_show (window1);

  ctk_main ();
  return 0;
}

