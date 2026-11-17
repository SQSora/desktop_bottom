import 'package:desktop_bottom_example/main.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('示例页展示桌面置底开关', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    expect(find.text('桌面置底'), findsOneWidget);
  });
}
